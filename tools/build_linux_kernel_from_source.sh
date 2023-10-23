if [! -f linux-6.2.15.tar.xz]; then
        wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.2.15.tar.xz
    fi
    tar xf linux-6.2.15.tar.xz
fi

pushd linux-6.2.15

    # Restore environment
    make distclean
    rm -rf `pwd`/../mykernel/install

    # Copy the configuration file of the Linux kernel from the host machine
    # The .config file is the kernel configuration file. If you want to add some features, you can modify this file directly or use 'make menuconfig' for visual editing.
    cp ../.config .config
    make olddefconfig

    # Add the following line to add compilation options
    ./scripts/config -e DEBUG_INFO -e DEBUG_KERNEL -e DEBUG_INFO_DWARF4
    # Enable compilation support for gdb. After adding this option, the compilation result will include './vmlinux-gdb.py'.
    ./scripts/config -e CONFIG_GDB_SCRIPTS
    # Notice! Disable address space layout randomization (ASLR), as this feature may prevent gdb from setting breakpoints.
    # It is recommended to disable this feature in 'make menuconfig'. It involves multiple configurations, which can be a bit cumbersome in the graphical interface, so I choose to explicitly disable it when starting the kernel in QEMU.

    # Choose the compression method for the kernel freely
    # ./scripts/config -d KERNEL_LZ4
    # ./scripts/config -e KERNEL_GZIP

    # Ensure virtualization support for booting the kernel, otherwise, there will be errors during compilation.
    # https://stackoverflow.com/questions/17242403/linux-running-self-compiled-kernel-in-qemu-vfs-unable-to-mount-root-fs-on-unk
    ./scripts/config -e CONFIG_VIRTIO_PCI
    ./scripts/config -e CONFIG_VIRTIO_BALLOON
    ./scripts/config -e CONFIG_VIRTIO_BLK
    ./scripts/config -e CONFIG_VIRTIO_NET
    ./scripts/config -e CONFIG_VIRTIO
    ./scripts/config -e CONFIG_VIRTIO_RING


    make CC=clang HOSTCC=clang -j 20 2>&1 | tee ../kernel_build.log

    # Generate a JSON file containing compilation options for clangd code completion
    ./scripts/clang-tools/gen_compile_commands.py

    # I plan to install it in this directory in the future, so I create a new folder first
    mkdir -p `pwd`/../mykernel/install
    # Install the kernel
    INSTALL_PATH=`pwd`/../mykernel/install make install 2>&1 | tee -a ../kernel_build.log

    # Install kernel modules
    INSTALL_MOD_PATH=`pwd`/../mykernel/modules_install make modules_install 2>&1 | tee -a ../kernel_build.log

    # Save vscode configuration files for subsequent code debugging
    cp -r ../.vscode .
    # Archive vmlinux
    cp vmlinux `pwd`/../mykernel/install

popd