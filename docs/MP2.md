# MP2：Multi-Programming

這份實作讓每個 `-e` 指定的 NOFF 程式擁有自己的 NachOS thread 與
address space。不同程式使用共用 bitmap 分配實體 frame，透過各自的
page table 對應虛擬頁。排程沿用原有的 FIFO ready queue 與 timer。

## 執行與測試

以下命令在 Mac 的專案根目錄執行。既有 `nachos-dev` 容器必須啟動，
並將本專案掛載至 `/workspace`。

```sh
python3 scripts/test_mp2.py
```

腳本會編譯 kernel 與三個測試程式，再檢查單程式結束、兩個程式在不同
timer seeds 下的記憶體與執行狀態、實際 thread 切換、frame 回收重用、
記憶體不足，以及不存在的 executable。

手動執行雙程式測試：

```sh
docker exec -w /workspace/nachos-4.0/code/test nachos-dev \
    make mp2_a.noff mp2_b.noff
docker exec -w /workspace/nachos-4.0/code/userprog nachos-dev \
    make nachos
docker exec -w /workspace/nachos-4.0/code/userprog nachos-dev \
    ./nachos -rs 1 -e ../test/mp2_a.noff -e ../test/mp2_b.noff
```

`mp2_a.c` 與 `mp2_b.c` 使用相同的虛擬配置，初始值分別是 111 與 222。
兩者反覆讀寫自己的陣列，成功時各印出該值，再呼叫 `Exit(0)`。
其 initialized data 長度超過一頁，也會檢查尾端的初始零值。
輸出 111 與 222 後，所有 threads 完成，NachOS 才停止。

`mp2_exit.c` 檢查 BSS 的初值，寫入非零值後從 `main()` 返回。
它用來驗證 startup stub 的 Exit 路徑，以及後續程式重用 frame 時的清零。
腳本在一次 NachOS 執行中啟動四份，累計分配量超過實體總容量，確認
釋放後確實能再分配，最後 bitmap 恢復為全部空閒。

原本的 `sort`、`matmult` 測試需要的記憶體可能超過這份設定的容量。
MP2 不提供 demand paging；容量不足會拒絕載入，不代表程式已成功執行。

## 修改說明

| 檔案 | 修改內容 |
| --- | --- |
| `userprog/userkernel.h`、`userkernel.cc` | 將每個 `-e` 的檔名存入 vector，為每個程式建立 thread，維護共用 frame bitmap。 |
| `userprog/addrspace.h`、`addrspace.cc` | 延後至 Load 分配 frame，建立獨立 page table，逐頁載入 code 與 initialized data，解構時歸還 frame。 |
| `userprog/exception.cc` | 處理 SC_Exit，解除 thread 與 address space 的關聯，回收記憶體，再結束目前 thread。 |
| `threads/thread.cc` | 修正恢復原 interrupt level 的註解，沒有改變排程行為。 |

程式需要的頁數由 NOFF 的 code、initialized data、uninitialized data 與
user stack 大小計算，再以 `PageSize` 向上取整數。Load 先確認 bitmap
剩餘容量，然後逐頁呼叫 `FindAndSet()`，將回傳的 frame 寫入 page table。
目前的分配迴圈沒有 NachOS 的阻塞或 yield 操作。

只能清空剛分配到的 frame。清空整個 `mainMemory` 會破壞其他程式的資料。
未從 executable 載入的 BSS 與 stack 由逐 frame 清零取得初始值。

`LoadSegment()` 每次讀取的長度是「目前頁的剩餘空間」與「segment 剩餘資料量」
的較小值。下一個虛擬頁可能對應不相鄰的 frame，因此跨頁後必須重新轉換位址。
code 與 initialized data 共用此流程。

`RestoreState()` 將目前程式的 page table 安裝到共用的 simulated machine。
User registers 的保存與恢復由原有 scheduler 呼叫 thread 的對應方法處理。

## Thread 執行流程 trace

加入 `-d t` 可以看到 thread 紀錄：

```sh
docker exec -w /workspace/nachos-4.0/code/userprog nachos-dev \
    ./nachos -d t -rs 1 -e ../test/mp2_a.noff -e ../test/mp2_b.noff
```

下面將 thread 名稱簡寫為 A 與 B；實際輸出使用 executable 路徑。

```text
Forking thread: A
Putting thread on ready list: A
Forking thread: B
Putting thread on ready list: B
Finishing thread: main
Sleeping thread: main
Switching from: main to: A
Beginning thread: A
Deleting thread: main
Yielding thread: A
Putting thread on ready list: A
Switching from: A to: B
Beginning thread: B
```

1. `ThreadedKernel::Initialize()` 為目前的啟動流程建立名為 main 的 Thread
   物件。`UserProgKernel::Run()` 呼叫 `Thread::Fork()`，準備新 thread 的
   kernel stack，並透過 `Scheduler::ReadyToRun()` 加入 ready queue 尾端。
2. main 呼叫 `ThreadedKernel::Run()`，進入 `Finish()` 與 `Sleep(TRUE)`。
   Scheduler 從 ready queue 前端取出下一個 thread，以 `SWITCH()` 切換。
3. 首次執行的新 thread 經過 `Thread::Begin()`，回收已結束的前一條 thread，
   然後執行 `RunUserProgram()`。該函式建立 AddrSpace 並呼叫 Execute。
4. Execute 載入 NOFF、設定 user registers、安裝 page table，再呼叫
   `Machine::Run()`，開始執行模擬的 MIPS user instructions。
5. Timer interrupt 經 `Alarm::CallBack()` 設定 `YieldOnReturn()`。
   Handler 返回時讓目前 thread Yield；若有另一條 ready thread，scheduler
   保存目前的 user registers，切換 kernel 執行狀態，並恢復接續 thread
   的 registers 與 page table。新 thread 的首次啟動則由 Execute 初始化。
6. User program 呼叫 Exit，或從 main 返回後經 startup stub 呼叫 Exit，
   會進入 `ExceptionHandler()` 的 SC_Exit。Handler 先將 thread 的 space
   設成 NULL，避免 scheduler 存取已釋放的 page table，再 delete AddrSpace，
   最後呼叫 `Finish()`。Thread 本身及其分配的 kernel stack 延後回收。
7. 沒有 runnable threads 時，`Interrupt::Idle()` 等待後續事件。
   沒有可繼續的工作與未來事件後，NachOS 正常停止。

`Finish()` 結束的是一條 NachOS thread，不是刪除 kernel 物件。
main thread 結束後，scheduler、machine、bitmap 與其他 threads 仍然存在。
在這份實作中，每個 user program 對應一條 thread；這不是所有 OS 的通則。

## 範圍

這份 MP2 使用原有排程器，沒有新增 MLFQ、priority scheduling、swap 或
demand paging。程式必須以可放入當前實體記憶體的 NOFF executable 執行。

Exit 目前結束 thread 並回收 frame，沒有提供 Join 或保存供父程式讀取的
exit status。MP1 的 open-file table 仍為 kernel 全域資料，沒有新增
每個 process 獨立的 file descriptor table。

這份文件記錄 trace 與程式修改，可作為規格中兩份報告的內容來源。
它不是課程平台要求的分組 PDF，也沒有代為上傳或繳交。
