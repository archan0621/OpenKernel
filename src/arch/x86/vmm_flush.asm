[bits 32]
global vmm_flush

; void vmm_flush(void* page_dir);
; CR3 레지스터에 페이지 디렉토리의 물리 주소를 설정하고 TLB를 플러시함
; 주의: page_dir는 물리 주소여야 함 (페이징 활성화 전에는 가상 주소 = 물리 주소)
vmm_flush:
    ; 로그는 C에서 출력하므로 여기서는 어셈블리만 수행
    mov eax, [esp + 4]   ; page_dir 주소 (매개변수 - 물리 주소여야 함)
    
    ; CR3에 물리 주소 설정 (하위 12비트는 무시됨, 4KB 정렬 필요)
    ; 페이징 활성화 전이므로 가상 주소 = 물리 주소
    mov cr3, eax
    
    ; TLB (Translation Lookaside Buffer) 플러시
    ; CR3을 다시 로드하면 TLB가 자동으로 플러시됨
    mov eax, cr3
    mov cr3, eax
    
    ret
