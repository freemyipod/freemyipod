thread_state = (
    "THREAD_FREE",
    "THREAD_SUSPENDED",
    "THREAD_READY",
    "THREAD_RUNNING",
    "THREAD_BLOCKED",
    "THREAD_DEFUNCT",
    "THREAD_DEFUNCT_ACK"
)

thread_block = (
    "THREAD_NOT_BLOCKED",
    "THREAD_BLOCK_SLEEP",
    "THREAD_BLOCK_MUTEX",
    "THREAD_BLOCK_WAKEUP",
    "THREAD_DEFUNCT_STKOV",
    "THREAD_DEFUNCT_PANIC"
)

thread_type = (
    "USER_THREAD",
    "OS_THREAD",
    "ORE_THREAD"
)

hwtypes = {
    0: "invalid",
    0x47324e49: "iPod nano 2g",
    0x47334e49: "iPod nano 3g",
    0x47344e49: "iPod nano 4g",
    0x4c435049: "iPod classic"
}

swtypes = {
    0: "invalid",
    1: "emBIOS Debugger"
}

responsecodes = {
    0: "invalid",
    1: "ok",
    2: "unsupported",
    3: "busy"
}