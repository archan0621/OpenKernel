; context_switch.asm
; 컨텍스트 스위칭 (x86 32-bit)
; ESP-only 방식: 스택이 곧 컨텍스트

[BITS 32]

; void switch_context(uint32_t** old_esp, uint32_t* new_esp);
; old_esp: 현재 esp를 저장할 포인터의 주소 (&task->esp)
; new_esp: 복원할 esp 값 (task->esp)
global switch_context

switch_context:
    ; 함수 진입 시:
    ; [esp]     = return address
    ; [esp + 4] = old_esp (uint32_t**)
    ; [esp + 8] = new_esp (uint32_t*)
    
    ; ✅ 정석: push 이전에 인자를 안전한 레지스터에 저장
    ; ecx, edx는 caller-saved이므로 자유롭게 사용 가능
    mov ecx, [esp + 4]      ; ecx = old_esp**
    mov edx, [esp + 8]      ; edx = new_esp*
    
    ; old_esp가 NULL이면 저장 건너뛰기
    test ecx, ecx
    jz .load_only
    
    ; === 저장 단계 ===
    ; 모든 레지스터를 스택에 푸시
    ; ⚠️ 주의: ecx와 edx에는 old_esp/new_esp가 들어있지만
    ; 원래 ecx/edx 레지스터 값은 이미 손상되었음
    ; 하지만 괜찮음 - 어차피 복원 시 새 값으로 덮어씌워질 것
    
    pushfd
    push ebp
    push edi
    push esi
    push edx                ; 여기 들어가는 건 new_esp 값이지만 상관없음
    push ecx                ; 여기 들어가는 건 old_esp** 값이지만 상관없음
    push ebx
    push eax
    
    ; 현재 esp를 old_esp에 저장
    ; ecx에는 여전히 old_esp** 값이 있음!
    mov [ecx], esp          ; *old_esp = 현재 esp
    
.load_only:
    ; === 복원 단계 ===
    ; edx에 new_esp가 이미 있음!
    mov esp, edx            ; 스택 전환
    
    ; 스택에서 레지스터 복원
    pop eax
    pop ebx
    pop ecx
    pop edx
    pop esi
    pop edi
    pop ebp
    popfd
    
    ret
