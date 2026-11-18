# 변경 이력

이 프로젝트의 주요 변경 사항을 기록합니다. 형식은 [Keep a Changelog](https://keepachangelog.com/ko/1.1.0/)를 따릅니다.

## 2026-11-18
### 문서
- README를 구조도와 흐름도 중심으로 다시 작성
- 요구사항 명세(SRS, ISO/IEC/IEEE 29148)와 설계 명세(SDS, IEEE 1016) 추가
- 검증을 마친 quirk와 DXYN 수용 기준을 완료로 표시

## 2026-07-23
### 추가
- 헤드리스 모드에 프레임 단위 키 누름/뗌 스크립트(`CHIP8_DUMP_KEYSEQ`) 추가
- n프레임마다 BMP를 추가로 저장하는 `CHIP8_DUMP_EVERY` 추가
- 프레임별 sound_timer를 기록하는 `CHIP8_BEEP_LOG` 추가
- 난수 시드를 고정하는 `CHIP8_SEED` 추가
- 키패드, Pong, Tetris 키 반응 검증 스냅샷 추가
### 문서
- README에 헤드리스 검증 훅 사용법과 검증 결과 추가

## 2026-07-13
### 추가
- C + SDL2 기반 원조 COSMAC VIP CHIP-8 에뮬레이터 (`./chip8 <rom>`)
- 옵코드 35개를 VIP 의미론으로 구현, quirk 6종은 VIP 값의 컴파일 타임 상수
- 64x32 화면 렌더링, 16키 키패드, 440Hz 사각파 비프
- CPU 700 IPS, 타이머 60Hz의 프레임 루프
- 잘못된 입력과 CPU 정지를 종료 코드로 구분
- 창 없이 실행 후 화면을 BMP로 저장하는 헤드리스 모드(`CHIP8_DUMP`, `CHIP8_DUMP_CYCLES`, `CHIP8_DUMP_KEY`)
- Timendus test suite, Pong, Tetris 검증 스냅샷
