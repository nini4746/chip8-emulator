# 설계 결정 기록 (ADR)

| 번호 | 결정 | 상태 | 날짜 |
|---|---|---|---|
| [0001](0001-vip-chip8-only.md) | 원조 COSMAC VIP CHIP-8만 지원 | 채택 | 2026-07-13 |
| [0002](0002-compile-time-quirk-constants.md) | quirk 6종을 컴파일 타임 상수로 분리하고 VIP 값으로 고정 | 채택 | 2026-07-13 |
| [0003](0003-separate-core-and-sdl-layer.md) | CPU 코어와 SDL 계층을 파일로 분리 | 채택 | 2026-07-13 |
| [0004](0004-60hz-frame-loop-sleep-pacing.md) | 60Hz 프레임 루프와 sleep 기반 페이싱 | 채택 | 2026-07-13 |
| [0005](0005-mask-memory-addresses.md) | 메모리 주소를 0x0FFF로 마스킹 | 채택 | 2026-07-13 |
| [0006](0006-callback-square-wave-audio.md) | 콜백 방식 440Hz 사각파 비프, 오디오 실패 시 무음으로 계속 | 채택 | 2026-07-13 |
| [0007](0007-env-gated-headless-hooks.md) | 환경 변수로 켜는 헤드리스 검증 훅 | 채택 | 2026-07-13 |
