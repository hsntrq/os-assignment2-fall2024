# Setup Notes

To get xv6 up and running on your machine, you'll need to install a few tools. Below, we'll guide you through the setup for macOS, Linux, and Windows Subsystem for Linux (WSL).

## The xv6 Source Code

You have the xv6 source code ready to build, first cd into `xv6-public`

```sh
cd xv6-public
```

## macOS Build Environment for xv6

To run xv6 on macOS, you'll need two main tools: `qemu` (a machine emulator) and a `gcc` cross-compilation toolchain. Here's how to install them:

### 1. Installing `qemu`

`qemu` emulates an x86 system, allowing you to boot and run xv6 without using a real machine. To install `qemu` on macOS, use MacPorts:

```sh
sudo port install qemu
```

This assumes that you have MacPorts installed. If you haven't already installed it, follow the instructions at the [MacPorts install page](https://www.macports.org/install.php). Ensure that `/opt/local/bin` (the typical MacPorts directory) is in your system's `PATH`.

Once installed, test if `qemu` is working by running:

```sh
qemu-system-x86_64 -nographic
```

Press `C-a x` to quit the emulator.

### 2. Installing the `gcc` Toolchain

Install the cross-compilation toolchain to compile xv6:

```sh
sudo port install i386-elf-gcc gdb
```

### 3. Building and Running xv6

Now, in the `xv6-public` directory, run:

```sh
make TOOLPREFIX=i386-elf- qemu-nox
```

If everything is installed correctly, you should see xv6 boot up in the terminal. To exit the emulator, press `C-a x`.

To simplify future builds, edit the `Makefile`:
- Uncomment and modify the `TOOLPREFIX` line as follows:

  ```sh
  TOOLPREFIX = i386-elf-
  ```

  This allows you to run `make qemu-nox` directly.
  
- Optionally, change the CPU count from 2 to 1 by modifying this line:

  ```sh
  CPUS := 2
  ```

  to:

  ```sh
  CPUS := 1
  ```

Now you're ready to explore xv6 on macOS!

## Linux and WSL (Windows Subsystem for Linux)

### 1. Installing `qemu`

On Linux or WSL, the process is similar. Use your package manager to install `qemu` and the necessary toolchain. For example, on Debian-based systems (including Ubuntu and WSL), you can run:

```sh
sudo apt update
sudo apt install qemu qemu-system-x86 gcc-multilib make
```

### 2. Installing `gcc` and `gdb`

To install the required compilers on Linux/WSL, use:

```sh
sudo apt install gcc gdb
```

### 3. Building and Running xv6

Once the tools are installed, navigate to its directory:

```sh
cd xv6-public
```

Run the following command to build and boot xv6:

```sh
make qemu-nox
```

If everything is set up correctly, xv6 will boot in the terminal. Use `C-a x` to quit the emulator.

### Optional Makefile Edits

To simplify the build command, as in the macOS section, you can modify the `Makefile`:

- Reduce the number of CPUs to 1 (recommended for simplicity):

  ```sh
  CPUS := 1
  ```

Now you can build and run xv6 easily with `make qemu-nox`.

### WSL Specific Notes

If you're using WSL (Windows Subsystem for Linux), ensure you have WSL 2 installed as it offers better performance and full system emulation support required by `qemu`. Follow [Microsoft's official documentation](https://docs.microsoft.com/en-us/windows/wsl/install) to set up WSL.

You can use a terminal emulator like Windows Terminal to work with WSL for a more native experience. Once you've installed the necessary packages as detailed above, the steps for building and running xv6 are identical to those for Linux.

Now you're ready to explore xv6 on Linux or WSL!
