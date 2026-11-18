# 0001. 원조 COSMAC VIP CHIP-8만 지원

- 상태: 채택
- 날짜: 2026-07-13

## 맥락
CHIP-8에는 원조 VIP 외에 SUPER-CHIP, XO-CHIP 같은 변종이 있다. 변종마다 옵코드와 해상도가 다르다. 학습 목표는 fetch-decode-execute 사이클과 레지스터, 메모리맵, 타이머를 익히는 것이고, 기간은 1주로 잡았다.

## 결정
- 원조 COSMAC VIP CHIP-8의 옵코드 35개만 VIP 의미론으로 구현한다.
- 화면은 64x32 모노크롬, 메모리 4KB, 스택 16레벨로 고정한다.
- 그 밖의 옵코드(SUPER-CHIP 포함)는 `unknown_opcode`로 처리해 `fatal: unknown opcode 0xNNNN at PC=0x...`를 출력하고 `CHIP8_HALT_UNKNOWN_OPCODE`로 정지한다. 창 모드는 종료 코드 5로 끝난다.

## 대안
- SUPER-CHIP, XO-CHIP 포함: 옵코드와 해상도가 달라 별개 프로젝트 분량이다.
- 모르는 옵코드를 조용히 무시: 오동작이 드러나지 않는다.

## 결과
- 장점: 범위가 Timendus test suite와 Pong, Tetris로 명확하다. 지원하지 않는 롬은 조용히 오동작하지 않고 즉시 정지한다.
- 단점/감수한 점: SUPER-CHIP, XO-CHIP 롬은 실행할 수 없다. 다음 확장 후보로 남긴다.
