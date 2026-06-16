# Open Kernel

방화벽을 만들던 보안 회사 재직 시절 자체 OS를 개발하며 어깨 넘어로 배운것들에 기반하여 운영체제 deepdive 학습을 목적으로 만들고 있습니다.   

POSIX 표준을 철저하게 준수하여 개발 할 예정입니다.

## Build

`Makefile`이 커널 빌드의 기준입니다.

```bash
make
```

빌드가 성공하면 ISO 이미지가 생성됩니다.

```text
build/archanOS.iso
```

전체를 깨끗하게 다시 빌드하려면 다음 명령을 사용합니다.

```bash
make rebuild
```

기존 `build.sh`도 같은 경로를 사용합니다.

```bash
./build.sh
```

## Run

QEMU로 실행하려면 다음 중 하나를 사용합니다.

```bash
make run
```

```bash
./run_qemu.sh
```

`run_qemu.sh`는 ISO가 없으면 먼저 `make`로 빌드한 뒤 QEMU를 실행합니다.

## Toolchain

현재 빌드는 다음 도구를 기대합니다.

- `nasm`
- `x86_64-elf-gcc`
- `x86_64-elf-ld`
- `i686-elf-grub-mkrescue`
- `qemu-system-i386`
