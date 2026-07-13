# Plan 04: 사운드와 최종 검증

## 이 plan의 목표

비프음을 붙이고, quirk 테스트와 실제 게임 (Pong, Tetris) 으로 전체를 최종 판정한다. SPEC의 R6 전체, R7 수용 기준 전체, R3의 클리핑 기준, 부정 수용 기준 4종 점검을 담당한다.

## 시작 전에 알아야 할 컨텍스트

- plan 03까지 만든 것: 완전한 CPU + 디스플레이 + 입력 + 타이밍. sound 타이머는 감소만 하고 소리는 아직 없음
- SDL2 오디오: SDL_OpenAudioDevice + 콜백 (또는 SDL_QueueAudio) 으로 사각파 하나 내보내면 충분. 파형 품질은 범위 외 - 들리기만 하면 된다
- 게임 롬 출처: Pong과 Tetris는 퍼블릭 도메인 CHIP-8 롬 아카이브에서 구한다 (예: https://github.com/kripod/chip8-roms 또는 johnearnest.github.io/chip8Archive). roms/에 배치
- 5-quirks 테스트: 실행 후 키 1로 "CHIP-8" 선택. 6개 항목 (vF reset, memory, disp.wait, clipping, shifting, jumping) 이 각각 판정된다. plan 02~03에서 VIP 값으로 구현했으므로 전부 정상이어야 한다 - 하나라도 틀리면 해당 plan의 quirk 상수/구현으로 돌아가서 고친다

## 해야 할 작업

1. 사운드: sound 타이머 > 0 이면 톤 재생, 0이면 정지
2. 5-quirks 실행해서 6개 항목 판정. 실패 항목이 있으면 원인 quirk를 추적해 수정
3. Pong, Tetris 각 60초 플레이 테스트
4. 부정 수용 기준 4종 최종 점검 (아래 체크리스트)
5. README에 빌드/실행/키 매핑 한 단락 추가 (선택이지만 권장)

## 빠지면 실패인 것

- 소리 시작/정지가 sound 타이머와 동기 (한 박자 늦게 꺼지는 건 실패)
- 5-quirks 6개 항목 전부 (5개만 맞으면 미완)

## 완료 판정

- [ ] `./chip8 roms/7-beep.ch8` 화면 표시와 동기해 비프음이 들림
- [ ] `./chip8 roms/5-quirks.ch8` CHIP-8 선택 시 6개 항목 전부 정상
- [ ] 클리핑 동작 확인 (5-quirks "clipping" 항목이 판정 - R3 기준)
- [ ] Pong: 60초 플레이, 크래시 없음, 매핑 키에 패들 반응, 화면 깨짐 없음
- [ ] Tetris: 60초 플레이, 크래시 없음, 매핑 키에 블록 반응, 화면 깨짐 없음

부정 기준 최종 점검:

- [ ] I/PC의 4KB 이탈이 프로세스 메모리를 침범하지 않음 (plan 02의 수제 롬 재실행)
- [ ] SUPER-CHIP 옵코드가 미지 옵코드 경로로 정지함 (예: 00FF를 넣은 수제 롬)
- [ ] 옵코드 구현 코드에 SDL 호출이 없음 (grep으로 확인: CPU 소스 파일에서 `SDL_` 검색 결과 0건)
- [ ] 실행 중 busy-spin 없음 (top 재확인)
