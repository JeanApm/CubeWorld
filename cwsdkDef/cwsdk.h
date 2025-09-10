#pragma once

#include "cwsdk\memory_management.h"
#include "cwsdk\cube.h"
#include "cwsdk\plasma.h"
#include "cwsdk\globals.h"

#define PUSH_ALL __asm {\
    __asm push eax          \
    __asm push ebx          \
    __asm push ecx          \
    __asm push edx          \
    __asm push esi          \
    __asm push edi          \
    __asm push ebp          \
    __asm push esp          \
}
#define POP_ALL __asm {\
    __asm pop esp          \
    __asm pop ebp          \
    __asm pop edi          \
    __asm pop esi          \
    __asm pop edx          \
    __asm pop ecx          \
    __asm pop ebx          \
    __asm pop eax          \
}

void WriteJMPFar(void* source, void* destination) {
    DWORD dwOldProtection;
    VirtualProtect(source, 5, PAGE_EXECUTE_READWRITE, &dwOldProtection);
    char* location = (char*)source;

    *((UINT8*)&location[0]) = 0xEA;
    *((UINT32*)&location[1]) = (UINT32)destination;

    VirtualProtect(location, 5, dwOldProtection, &dwOldProtection);
}

void WriteJMP(void* source, void* destination, uint32_t size = 5) {
    DWORD dwOldProtection;
    VirtualProtect(source, size, PAGE_EXECUTE_READWRITE, &dwOldProtection);
    char* location = (char*)source;

    *((UINT8*)&location[0]) = 0xE9;
    *((UINT32*)&location[1]) = (UINT32)destination - (UINT32)source - 5;
    for (uint32_t i = 5; i < size; i++) {
        *((UINT8*)&location[i]) = 0x90;
    }

    VirtualProtect(location, size, dwOldProtection, &dwOldProtection);
}

void WriteCALL(void* source, void* destination) {
    DWORD dwOldProtection;
    VirtualProtect(source, 5, PAGE_EXECUTE_READWRITE, &dwOldProtection);
    char* location = (char*)source;

    *((UINT8*)&location[0]) = 0xE8;
    *((UINT32*)&location[1]) = (UINT32)destination - (UINT32)source - 5;

    VirtualProtect(location, 5, dwOldProtection, &dwOldProtection);
}

void WriteBYTE(void* source, uint8_t byte) {
    DWORD dwOldProtection;
    VirtualProtect(source, 1, PAGE_EXECUTE_READWRITE, &dwOldProtection);
    char* location = (char*)source;

    *((UINT8*)&location[0]) = byte;

    VirtualProtect(location, 1, dwOldProtection, &dwOldProtection);
}