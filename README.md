# CHIP-8 에뮬레이터

> 토이 1번. 예상 1주. 언어: C (SDL2).

## CS 지식

CPU 동작 원리 - fetch-decode-execute 사이클, 레지스터, 메모리맵, 타이머.

## 왜

- 42/기존 프로젝트에 CPU 층이 없음 (visual-disk는 디스크, cub3d/miniRT는 그래픽)
- 에뮬레이터 입문 정석: 옵코드 35개, 명세 완전 공개, 돌릴 게임 롬 풍부
- xv6 커널 트랙 가기 전 "기계 밑바닥" 감각 워밍업
- 토이 중 가장 "게임"에 가까운 재미 - 시동용

## 구성

- 메모리 4KB, 레지스터 V0~VF, 스택, 딜레이/사운드 타이머
- 옵코드 디코딩 루프
- 64x32 모노크롬 디스플레이 (SDL2)
- 키패드 16키 입력
- 테스트 롬 → Pong/Tetris 구동이 완료 조건

## 헤드리스 검증 훅 (env-gated, 미설정 시 동작 무변경)

- `CHIP8_DUMP=<path.bmp>` - 창 없이 실행 후 프레임버퍼를 BMP로 덤프
- `CHIP8_DUMP_CYCLES=<n>` - 덤프 전 실행할 명령 수 (기본 600000)
- `CHIP8_DUMP_KEY=<hex>` - 단일 키를 초반에 hold 후 release (메뉴 진입용)
- `CHIP8_DUMP_KEYSEQ="F:K:d,F:K:u,..."` - 프레임 F에 키 K(hex) press(d)/release(u) 스크립트
- `CHIP8_DUMP_EVERY=<n>` - n프레임마다 `<path>.f<frame>.bmp` 추가 덤프
- `CHIP8_BEEP_LOG=<file>` - 프레임별 `frame sound_timer 점등픽셀수` 트레이스
- `CHIP8_SEED=<n>` - CXNN RNG 고정 (재현 가능한 비교 런)
- 실제 오디오 출력 검증: `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE=out.raw ./chip8 roms/7-beep.ch8` → out.raw가 S16 mono 44.1kHz 파형 (440Hz 정사각파 확인됨)

인터랙티브 항목 검증 결과(2026-07-14): 6-keypad 3종(EX9E/EXA1/FX0A 릴리스 판정) 통과, 7-beep 화면-사운드 프레임 동기 + 440Hz 파형 실측, Pong/Tetris 60초분 무크래시·키 반응 - 증거는 `snapshots/6-keypad-*.png`, `snapshots/*-keyresponse.png`, 상세는 `.planning/SPEC.md` 수용 기준 주석.

## 다음 확장 (선택)

재미 붙으면 Game Boy 에뮬레이터가 정석 다음 단계. 또는 바이트코드 VM(토이 7번)과 개념 연결 - 둘 다 "가상 머신".
