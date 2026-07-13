# Plan 02: 명령어 세트 완성

## 이 plan의 목표

옵코드 35개 전부를 VIP 의미론으로 구현해서 corax+와 flags 테스트를 통과시킨다. SPEC의 R2 전체와 R3의 VF 충돌 기준, R7의 quirk 상수 분리 (작업만, 판정은 plan 04) 를 담당한다.

## 시작 전에 알아야 할 컨텍스트

- plan 01이 만든 것: 상태 구조체, ROM 로더, SDL 렌더링, 디스패치 골격 + 옵코드 6종 (00E0, 1NNN, 6XNN, 7XNN, ANNN, DXYN), 미지 옵코드 정지 경로
- 이 plan은 헤드리스에 가깝다 - 테스트 롬이 결과를 화면에 그려주므로 새 SDL 작업은 없음
- 옵코드 표: Cowgod's Chip-8 Technical Reference 또는 Tobias Langhoff의 "Guide to making a CHIP-8 emulator" 참고. 단, 두 자료가 갈리는 지점 (quirk) 은 아래 VIP 값이 정답
- SPEC이 고정한 VIP quirk 값 (컴파일 타임 상수로 분리할 것):
  - 8XY1/8XY2/8XY3: 실행 후 VF = 0
  - 8XY6/8XYE: VY를 시프트해서 VX에 저장 (VX 제자리 시프트 아님)
  - FX55/FX65: 실행 후 I = I + X + 1
  - BNNN: NNN + V0
- 함정: VF가 피연산자이면서 플래그 목적지인 경우 (예: 8FF4). 결과를 먼저 쓰고 플래그를 나중에 써야 플래그가 남는다. 4-flags가 이걸 잡아낸다

## 해야 할 작업

1. quirk 상수를 헤더에 이름 붙여 분리 (예: QUIRK_VF_RESET, QUIRK_SHIFT_VY, QUIRK_MEMORY_INCREMENT_I, QUIRK_JUMP_V0). 전부 VIP 값으로 고정
2. 나머지 옵코드 구현. 권장 순서: 흐름 제어 (2NNN/00EE, 3/4/5/9XY0) → 산술/논리 (8XY0~8XYE) → BNNN, CXNN → 메모리 (FX1E, FX29, FX33, FX55, FX65) → 타이머/키 옵코드 (FX07/FX15/FX18, EX9E/EXA1/FX0A) 는 껍데기만 (타이머와 키패드 배열 읽고 쓰기만, 실제 60Hz 감소와 SDL 키 이벤트는 plan 03)
3. 스택 push/pop에 경계 검사: 오버플로/언더플로 시 PC 포함 에러 메시지 출력 후 정지
4. I와 PC의 메모리 접근에 4KB 방어 (마스킹 또는 에러 정지 - 부정 기준)

## 빠지면 실패인 것

- quirk 4종이 VIP 값인 것 (모던 값으로 구현하면 plan 04의 5-quirks에서 전멸한다)
- 플래그 기록이 결과 기록보다 나중인 것
- CXNN의 난수 시드 (매번 같은 난수면 게임이 이상해진다)
- FX33 BCD의 자릿수 순서 (백/십/일 순으로 I, I+1, I+2)

## 완료 판정

- [ ] `./chip8 roms/3-corax+.ch8` 전 항목 체크 표시 (X 표시 0개)
- [ ] `./chip8 roms/4-flags.ch8` 전 항목 체크 표시
- [ ] DXYN 충돌 시 VF=1, 아니면 VF=0 (corax+가 판정)
- [ ] 스택 오버플로/언더플로 시 에러 메시지 (PC 포함) 출력 후 정지 (2NNN만 반복하는 수제 롬 몇 바이트로 확인)
- [ ] 미지 옵코드에 PC와 옵코드 출력 후 정지 (수제 롬로 확인, 예: 0xFFFF)
- [ ] I가 4KB를 벗어나는 접근에서 크래시/침범 없음 (ANNN으로 0xFFF 설정 후 FX65 하는 수제 롬으로 확인)
