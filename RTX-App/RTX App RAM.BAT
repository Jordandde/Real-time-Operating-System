SET PATH=D:\Software\keil_v5\ARM\ARMCC\Bin;C:\Program Files (x86)\Common Files\Oracle\Java\javapath;C:\Python39\Scripts\;C:\Python39\;C:\Program Files (x86)\Razer Chroma SDK\bin;C:\Program Files\Razer Chroma SDK\bin;C:\Program Files (x86)\Razer\ChromaBroadcast\bin;C:\Program Files\Razer\ChromaBroadcast\bin;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Windows\System32\OpenSSH\;C:\Program Files (x86)\NVIDIA Corporation\PhysX\Common;C:\Program Files\NVIDIA Corporation\NVIDIA NvDLISR;C:\Users\16478\AppData\Local\Microsoft\WindowsApps;C:\Windows\system32\config\systemprofile\AppData\Local\Microsoft\WindowsApps;C:\Program Files\nodejs\;C:\ProgramData\chocolatey\bin;C:\Program Files\dotnet\;C:\Program Files\Git\cmd;D:\Program Files\PuTTY\;D:\Program Files\MATLAB\R2022a\bin;C:\Users\16478\AppData\Local\Microsoft\WindowsApps;;C:\Users\16478\AppData\Local\Programs\Microsoft VS Code\bin;C:\Users\16478\AppData\Roaming\npm
SET CPU_TYPE=LPC1768
SET CPU_VENDOR=NXP
SET UV2_TARGET=RTX App RAM
SET CPU_CLOCK=0x00B71B00
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\uart_polling.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\uart_irq.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\timer.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\hal.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\k_mem.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\k_msg.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\k_rtx_init.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\k_task.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\printf.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\cdisp_task.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\kcd_task.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\null_task.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\wall_clock_task.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\tsk_util.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmAsm" --Via ".\objects\ram\startup_lpc17xx._ia"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\system_lpc17xx.__i"
"D:\Software\keil_v5\ARM\ARMCC\Bin\ArmLink" --Via ".\Objects\RAM\rtx-app-ram.lnp"
