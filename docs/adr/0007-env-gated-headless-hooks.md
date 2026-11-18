# 0007. 환경 변수로 켜는 헤드리스 검증 훅

- 상태: 채택
- 날짜: 2026-07-13

## 맥락
Timendus 롬의 판정 결과는 화면에 표시된다. 화면, 키 입력, 소리를 사람 없이 확인할 방법이 필요했다. 검증용 코드가 일반 실행 경로를 바꾸면 안 된다.

## 결정
- `CHIP8_DUMP`가 설정되면 `run_headless`로 창 없이 실행하고, 끝나면 프레임버퍼를 `SDL_SaveBMP`로 저장한다(512x256).
- 헤드리스 루프는 창 모드와 같은 프레임 구조를 따른다. 프레임당 11 명령, display wait, 타이머 1회 감소.
- 옵션은 모두 환경 변수다: `CHIP8_DUMP_CYCLES`, `CHIP8_DUMP_KEY`, 이후 `CHIP8_DUMP_KEYSEQ`, `CHIP8_DUMP_EVERY`, `CHIP8_BEEP_LOG`, `CHIP8_SEED` 추가(2026-07-23).

## 대안
- 별도 테스트 바이너리, CLI 플래그: 채택하지 않았다. 환경 변수는 미설정 시 일반 실행 경로를 바꾸지 않고, 같은 바이너리로 검증할 수 있다.
- 헤드리스에서 명령만 연속 실행: display wait와 타이머 비율이 달라져 5-quirks가 display wait를 잘못 판정한다.

## 결과
- 장점: Timendus 1~7, Pong, Tetris 검증을 스냅샷(`snapshots/`)으로 남길 수 있다. 키 주입과 고정 시드로 키 반응을 재현 가능하게 비교할 수 있다.
- 단점/감수한 점: `CHIP8_DUMP_KEY`와 `CHIP8_DUMP_KEYSEQ`를 같은 키에 함께 쓰는 것은 지원하지 않는다. `CHIP8_DUMP_KEYSEQ` 형식이 틀리면 오류 없이 이벤트 0개로 처리한다.
