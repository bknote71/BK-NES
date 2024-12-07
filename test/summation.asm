        .org $8000               ; PRG-ROM 시작 주소 ($8000)

start:
        LDX #$05                ; X 레지스터에 5를 로드 (합산할 최대값)
        JSR sum                 ; 합 계산 서브루틴 호출

        ; 종료 확인을 위한 메모리 업데이트
        LDA result              ; 결과를 A 레지스터에 로드
        STA $F001               ; 결과를 $F001에 저장 (종료 조건)

        BRK                     ; 프로그램 종료 (디버깅용)

; 합 계산 서브루틴
sum:
        TXA                     ; X를 A로 이동
        BEQ done                ; X == 0이면 done으로 점프
        PHA                     ; A를 스택에 저장
        DEX                     ; X = X - 1
        JSR sum                 ; 재귀 호출
        PLA                     ; 스택에서 값 복구
        CLC                     ; Carry 플래그 클리어
        ADC result              ; 현재 결과에 A 추가
        STA result              ; 결과 저장
        RTS                     ; 서브루틴 복귀

; 재귀 종료 루틴
done:
        LDA #$00                ; A = 0
        STA result              ; 결과에 0 저장
        RTS                     ; 서브루틴 복귀

; 결과 저장 위치
result: .byte $00               ; 합산 결과를 저장할 메모리

; 리셋 벡터 설정
        .org $FFFA              ; NES 리셋/IRQ/NMI 벡터
        .word 0                 ; NMI 벡터 (사용하지 않음)
        .word 0                 ; IRQ 벡터 (사용하지 않음)
        .word start             ; 리셋 벡터 (프로그램 시작 주소)
