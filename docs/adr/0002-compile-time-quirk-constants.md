# 0002. quirk 6종을 컴파일 타임 상수로 분리하고 VIP 값으로 고정

- 상태: 채택
- 날짜: 2026-07-13

## 맥락
CHIP-8 인터프리터마다 일부 옵코드 동작(quirk)이 다르다. 목표 롬(Timendus 5-quirks, Pong, Tetris)은 VIP 동작을 기준으로 판정한다. 학습 후에는 값을 뒤집어 차이를 직접 확인해 보고 싶다.

## 결정
`chip8.h`에 quirk 6종을 `#define` 상수로 두고 모두 VIP 값(1)으로 고정한다.

- `QUIRK_VF_RESET`: 8XY1/8XY2/8XY3 실행 후 VF=0
- `QUIRK_MEMORY_INCREMENT_I`: FX55/FX65 실행 후 I = I + X + 1
- `QUIRK_SHIFT_VY`: 8XY6/8XYE는 VY를 시프트해서 VX에 저장
- `QUIRK_CLIPPING`: DXYN 스프라이트는 가장자리에서 클리핑
- `QUIRK_DISPLAY_WAIT`: DXYN은 프레임당 1회만 실행
- `QUIRK_JUMP_V0`: BNNN은 NNN + V0으로 점프

## 대안
- 런타임 CLI 옵션 / 설정 파일: 학습 목적에는 컴파일 타임 상수로 충분하다.

## 결과
- 장점: 옵코드 코드를 뒤지지 않고 상수 하나로 동작을 바꿔 볼 수 있다. Timendus 5-quirks에서 6개 항목이 모두 VIP와 일치한다.
- 단점/감수한 점: quirk를 바꾸려면 다시 빌드해야 한다. FX1E의 VF 변경(Amiga quirk)은 목표 롬이 쓰지 않아 넣지 않았다.
