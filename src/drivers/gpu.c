#include "../include/gpu.h"
#include "../include/pci.h"
#include "../include/io.h"
#include "../include/string.h"
#include <stddef.h>

// External function declarations
extern void terminal_writestring(const char* data);
extern char* itoa(int value, char* str, int base);

// Global variables
static gpu_device_t gpu_devices[8];
static int gpu_count = 0;

// VGA detection and basic operations
uint8_t gpu_read_vga_register(uint16_t port) {
    return inb(port);
}

void gpu_write_vga_register(uint16_t port, uint8_t value) {
    outb(port, value);
}

// Get vendor name from vendor ID
const char* gpu_get_vendor_name(uint16_t vendor_id) {
    switch (vendor_id) {
        case GPU_VENDOR_NVIDIA:     return "NVIDIA Corporation";
        case GPU_VENDOR_AMD:        return "Advanced Micro Devices";
        case GPU_VENDOR_INTEL:      return "Intel Corporation";
        case GPU_VENDOR_MATROX:     return "Matrox Graphics";
        case GPU_VENDOR_3DFX:       return "3dfx Interactive";
        case GPU_VENDOR_S3:         return "S3 Graphics";
        case GPU_VENDOR_CIRRUS:     return "Cirrus Logic";
        case GPU_VENDOR_TRIDENT:    return "Trident Microsystems";
        case GPU_VENDOR_TSENG:      return "Tseng Labs";
        case GPU_VENDOR_SIS:        return "Silicon Integrated Systems";
        case GPU_VENDOR_VIA:        return "VIA Technologies";
        case GPU_VENDOR_VMWARE:     return "VMware Inc.";
        case GPU_VENDOR_VIRTUALBOX: return "Oracle VirtualBox";
        case GPU_VENDOR_QEMU:       return "QEMU";
        default:                    return "Unknown Vendor";
    }
}

// Get device name from vendor and device ID
const char* gpu_get_device_name(uint16_t vendor_id, uint16_t device_id) {
    switch (vendor_id) {
        case GPU_VENDOR_NVIDIA:
            switch (device_id) {
                case 0x0020: return "RIVA TNT";
                case 0x0028: return "RIVA TNT2";
                case 0x0029: return "RIVA TNT2 Ultra";
                case 0x0100: return "GeForce 256";
                case 0x0101: return "GeForce DDR";
                case 0x0110: return "GeForce2 MX/MX 400";
                case 0x0150: return "GeForce2 GTS";
                case 0x0200: return "GeForce3";
                case 0x0250: return "GeForce4 Ti 4600";
                case 0x0251: return "GeForce4 Ti 4400";
                case 0x0253: return "GeForce4 Ti 4200";
                case 0x0300: return "GeForce FX 5800 Ultra";
                case 0x0301: return "GeForce FX 5800";
                case 0x0302: return "GeForce FX 5600 Ultra";
                case 0x0040: return "GeForce 6800 Ultra";
                case 0x0041: return "GeForce 6800";
                case 0x0042: return "GeForce 6800 LE";
                case 0x0043: return "GeForce 6800 XE";
                case 0x0090: return "GeForce 7800 GTX";
                case 0x0091: return "GeForce 7800 GTX";
                case 0x0092: return "GeForce 7800 GT";
                case 0x0191: return "GeForce 8800 GTX";
                case 0x0193: return "GeForce 8800 GTS";
                case 0x0194: return "GeForce 8800 Ultra";
                case 0x0400: return "GeForce 8600 GTS";
                case 0x0401: return "GeForce 8600 GT";
                case 0x0402: return "GeForce 8600 GT";
                case 0x0640: return "GeForce 9500 GT";
                case 0x0641: return "GeForce 9400 GT";
                case 0x0DE0: return "GeForce GT 440";
                case 0x0DE1: return "GeForce GT 430";
                case 0x0DE2: return "GeForce GT 420";
                case 0x1180: return "GeForce GTX 680";
                case 0x1181: return "GeForce GTX 670";
                case 0x1182: return "GeForce GTX 660 Ti";
                case 0x1183: return "GeForce GTX 660";
                case 0x1380: return "GeForce GTX 750 Ti";
                case 0x1381: return "GeForce GTX 750";
                case 0x1B80: return "GeForce GTX 1080";
                case 0x1B81: return "GeForce GTX 1070";
                case 0x1B82: return "GeForce GTX 1070 Ti";
                case 0x1B83: return "GeForce GTX 1060 6GB";
                case 0x1C02: return "GeForce GTX 1060 3GB";
                case 0x1C03: return "GeForce GTX 1060 6GB";
                case 0x1C20: return "GeForce GTX 1060";
                case 0x1F02: return "GeForce RTX 2080 Ti";
                case 0x1F03: return "GeForce RTX 2080 SUPER";
                case 0x1F04: return "GeForce RTX 2080";
                case 0x1F06: return "GeForce RTX 2070 SUPER";
                case 0x1F07: return "GeForce RTX 2070";
                case 0x1F08: return "GeForce RTX 2060 SUPER";
                case 0x1F09: return "GeForce RTX 2060";
                case 0x2182: return "GeForce RTX 3060 Ti";
                case 0x2184: return "GeForce RTX 3070";
                case 0x2204: return "GeForce RTX 3080";
                case 0x2206: return "GeForce RTX 3080 Ti";
                case 0x2208: return "GeForce RTX 3080 Ti";
                case 0x220A: return "GeForce RTX 3080";
                case 0x2216: return "GeForce RTX 3080";
                case 0x2230: return "GeForce RTX 4090";
                case 0x2231: return "GeForce RTX 4080";
                case 0x2232: return "GeForce RTX 4070 Ti";
                case 0x2233: return "GeForce RTX 4070";
                default: return "NVIDIA Graphics Card";
            }
            
        case GPU_VENDOR_AMD:
            switch (device_id) {
                case 0x4C57: return "Radeon RV200 [Mobility 7500]";
                case 0x4C58: return "Radeon RV200 [Mobility FireGL 7800]";
                case 0x4C59: return "Radeon RV100 [Radeon 7000/VE]";
                case 0x4C5A: return "Radeon RV100 [Radeon 7000/VE]";
                case 0x4C64: return "Radeon RV250 [Radeon 9000]";
                case 0x4C65: return "Radeon RV250 [Radeon 9000]";
                case 0x4C66: return "Radeon RV250 [Radeon 9000]";
                case 0x4C67: return "Radeon RV250 [Radeon 9000]";
                case 0x5144: return "Radeon R100 [Radeon 7200]";
                case 0x5145: return "Radeon R100 [Radeon 7200]";
                case 0x5146: return "Radeon R100 [Radeon 7200]";
                case 0x5147: return "Radeon R100 [Radeon 7200]";
                case 0x5148: return "Radeon R200 [Radeon 8500/8500LE]";
                case 0x5149: return "Radeon R200 [Radeon 8500]";
                case 0x514A: return "Radeon R200 [Radeon 8500]";
                case 0x514B: return "Radeon R200 [Radeon 8500]";
                case 0x514C: return "Radeon R200 [Radeon 8500]";
                                case 0x514D: return "Radeon R200 [Radeon 9100]";
                case 0x514E: return "Radeon R200 [Radeon 9100]";
                case 0x514F: return "Radeon R200 [Radeon 9100]";
                case 0x5157: return "Radeon RV200 [Radeon 7500/7500 LE]";
                case 0x5158: return "Radeon RV200 [Radeon 7500/7500 LE]";
                case 0x5159: return "Radeon RV100 [Radeon 7000/VE]";
                case 0x515A: return "Radeon RV100 [Radeon 7000/VE]";
                case 0x5245: return "Rage 128 GL PCI";
                case 0x5246: return "Rage 128 VR AGP";
                case 0x5247: return "Rage 128 VR AGP";
                case 0x524B: return "Rage 128 VR AGP";
                case 0x524C: return "Rage 128 VR AGP";
                case 0x5961: return "Radeon RV280 [Radeon 9200]";
                case 0x5962: return "Radeon RV280 [Radeon 9200]";
                case 0x5964: return "Radeon RV280 [Radeon 9200 SE]";
                case 0x5965: return "Radeon RV280 [Radeon 9200 SE]";
                case 0x5969: return "Radeon ES1000";
                case 0x596A: return "Radeon ES1000";
                case 0x596B: return "Radeon ES1000";
                case 0x5B60: return "Radeon RV370 [Radeon X300]";
                case 0x5B62: return "Radeon RV370 [Radeon X600]";
                case 0x5B63: return "Radeon RV370 [Radeon X550]";
                case 0x5B64: return "Radeon RV370 [FireGL V3100]";
                case 0x5B65: return "Radeon RV370 [FireGL D1100]";
                case 0x5E48: return "Radeon RV410 [Radeon X700]";
                case 0x5E49: return "Radeon RV410 [Radeon X700/X700 Pro]";
                case 0x5E4A: return "Radeon RV410 [Radeon X700 XT]";
                case 0x5E4B: return "Radeon RV410 [Radeon X700 Pro]";
                case 0x5E4C: return "Radeon RV410 [Radeon X700 SE]";
                case 0x5E4D: return "Radeon RV410 [Radeon X700]";
                case 0x5E4F: return "Radeon RV410 [Radeon X700 SE]";
                case 0x6738: return "Barts XT [Radeon HD 6870]";
                case 0x6739: return "Barts PRO [Radeon HD 6850]";
                case 0x6778: return "Caicos [Radeon HD 7470/8470 / R5 235/310 OEM]";
                case 0x6779: return "Caicos [Radeon HD 6450/7450/8450 / R5 230 OEM]";
                case 0x677B: return "Caicos PRO [Radeon HD 7450]";
                case 0x6798: return "Tahiti XT [Radeon HD 7970/8970 OEM / R9 280X]";
                case 0x6799: return "Tahiti PRO [Radeon HD 7950/8950 OEM / R9 280]";
                case 0x679A: return "Tahiti PRO [Radeon HD 7950/8950 OEM / R9 280]";
                case 0x679B: return "Tahiti PRO [Radeon HD 7950 Mac Edition]";
                case 0x679E: return "Tahiti LE [Radeon HD 7870 XT]";
                case 0x67B0: return "Hawaii XT / Grenada XT [Radeon R9 290X/390X]";
                case 0x67B1: return "Hawaii PRO [Radeon R9 290/390]";
                case 0x67B9: return "Vesuvius [Radeon R9 295X2]";
                case 0x67DF: return "Ellesmere [Radeon RX 470/480/570/570X/580/580X/590]";
                case 0x67EF: return "Baffin [Radeon RX 460/560D / Pro 450/455/460/555/555X/560/560X]";
                case 0x6FDF: return "Polaris 20 XL [Radeon RX 580 2048SP]";
                case 0x7300: return "Fiji [Radeon R9 FURY / NANO Series]";
                case 0x730F: return "Fiji [Radeon R9 FURY X / NANO]";
                case 0x731F: return "Navi 10 [Radeon RX 5700 XT/5700]";
                case 0x7340: return "Navi 14 [Radeon RX 5500/5500M / Pro 5500M]";
                case 0x73BF: return "Navi 21 [Radeon RX 6800/6800 XT / 6900 XT]";
                case 0x73DF: return "Navi 22 [Radeon RX 6700/6700 XT/6750 XT / 6800M/6850M XT]";
                case 0x73EF: return "Navi 23 [Radeon RX 6600/6600 XT/6600M]";
                case 0x73FF: return "Navi 23 [Radeon RX 6600/6600 XT/6600M]";
                case 0x744C: return "Navi 31 [Radeon RX 7900 XT/7900 XTX/7900M]";
                case 0x7448: return "Navi 32 [Radeon RX 7700 XT / 7800 XT]";
                case 0x7479: return "Navi 33 [Radeon RX 7600/7600 XT/7600M XT/7600S/7700S / PRO W7600]";
                default: return "AMD Graphics Card";
            }
            
        case GPU_VENDOR_INTEL:
            switch (device_id) {
                case 0x7121: return "810 Chipset Graphics Controller";
                case 0x7123: return "810 DC-100 Chipset Graphics Controller";
                case 0x7125: return "810e Chipset Graphics Controller";
                case 0x1132: return "815 Chipset Graphics Controller";
                case 0x3577: return "830M/845G/852GM/855GM/865G Graphics Controller";
                case 0x2562: return "845G/GL Chipset Integrated Graphics Device";
                case 0x3582: return "852GM/855GM Integrated Graphics Device";
                case 0x2572: return "865G Integrated Graphics Controller";
                case 0x2582: return "915G/GV/910GL Integrated Graphics Controller";
                case 0x258A: return "E7221 Integrated Graphics Controller";
                case 0x2592: return "Mobile 915GM/GMS/910GML Express Graphics Controller";
                case 0x2772: return "945G/GZ Integrated Graphics Controller";
                case 0x27A2: return "Mobile 945GM/GMS/GME, 943/940GML Express Integrated Graphics Controller";
                case 0x27AE: return "Mobile 945GSE Express Integrated Graphics Controller";
                case 0x29B2: return "Q35 Express Integrated Graphics Controller";
                case 0x29C2: return "G33/G31 Express Integrated Graphics Controller";
                case 0x29D2: return "Q33 Express Integrated Graphics Controller";
                case 0x2A02: return "Mobile GM965/GL960 Integrated Graphics Controller (primary)";
                case 0x2A12: return "Mobile GME965/GLE960 Integrated Graphics Controller (primary)";
                case 0x2A42: return "Mobile 4 Series Chipset Integrated Graphics Controller";
                case 0x2E02: return "4 Series Chipset Integrated Graphics Controller";
                case 0x2E12: return "Q45/Q43 Chipset Integrated Graphics Controller";
                case 0x2E22: return "G45/G43 Chipset Integrated Graphics Controller";
                case 0x2E32: return "G41 Chipset Integrated Graphics Controller";
                case 0x0042: return "Core Processor Integrated Graphics Controller";
                case 0x0046: return "Core Processor Integrated Graphics Controller";
                case 0x0102: return "2nd Generation Core Processor Family Integrated Graphics Controller";
                case 0x0106: return "2nd Generation Core Processor Family Integrated Graphics Controller";
                case 0x010A: return "2nd Generation Core Processor Family Integrated Graphics Controller";
                case 0x0112: return "2nd Generation Core Processor Family Integrated Graphics Controller";
                case 0x0116: return "2nd Generation Core Processor Family Integrated Graphics Controller";
                case 0x0122: return "2nd Generation Core Processor Family Integrated Graphics Controller";
                case 0x0126: return "2nd Generation Core Processor Family Integrated Graphics Controller";
                case 0x0152: return "Xeon E3-1200 v2/3rd Gen Core processor Graphics Controller";
                case 0x0156: return "3rd Gen Core processor Graphics Controller";
                case 0x015A: return "Xeon E3-1200 v2/3rd Gen Core processor Graphics Controller";
                case 0x0162: return "Xeon E3-1200 v2/3rd Gen Core processor Graphics Controller";
                case 0x0166: return "3rd Gen Core processor Graphics Controller";
                case 0x016A: return "Xeon E3-1200 v2/3rd Gen Core processor Graphics Controller";
                case 0x0402: return "Xeon E3-1200 v3/4th Gen Core Processor Integrated Graphics Controller";
                case 0x0406: return "4th Gen Core Processor Integrated Graphics Controller";
                case 0x040A: return "Xeon E3-1200 v3/4th Gen Core Processor Integrated Graphics Controller";
                case 0x0412: return "Xeon E3-1200 v3/4th Gen Core Processor Integrated Graphics Controller";
                case 0x0416: return "4th Gen Core Processor Integrated Graphics Controller";
                case 0x041A: return "Xeon E3-1200 v3/4th Gen Core Processor Integrated Graphics Controller";
                case 0x041E: return "4th Generation Core Processor Family Integrated Graphics Controller";
                case 0x0422: return "Xeon E3-1200 v3/4th Gen Core Processor Integrated Graphics Controller";
                case 0x0426: return "4th Gen Core Processor Integrated Graphics Controller";
                case 0x042A: return "Xeon E3-1200 v3/4th Gen Core Processor Integrated Graphics Controller";
                case 0x0A02: return "Haswell-ULT Integrated Graphics Controller";
                case 0x0A06: return "Haswell-ULT Integrated Graphics Controller";
                case 0x0A0A: return "Haswell-ULT Integrated Graphics Controller";
                case 0x0A0B: return "Haswell-ULT Integrated Graphics Controller";
                case 0x0A0E: return "Haswell-ULT Integrated Graphics Controller";
                case 0x0A16: return "Haswell-ULT Integrated Graphics Controller";
                case 0x0A1E: return "Haswell-ULT Integrated Graphics Controller";
                case 0x0A26: return "Haswell-ULT Integrated Graphics Controller";
                case 0x0A2E: return "Haswell-ULT Integrated Graphics Controller";
                case 0x0D02: return "Crystal Well Integrated Graphics Controller";
                case 0x0D06: return "Crystal Well Integrated Graphics Controller";
                case 0x0D0A: return "Crystal Well Integrated Graphics Controller";
                case 0x0D0B: return "Crystal Well Integrated Graphics Controller";
                case 0x0D0E: return "Crystal Well Integrated Graphics Controller";
                case 0x0D16: return "Crystal Well Integrated Graphics Controller";
                case 0x0D1E: return "Crystal Well Integrated Graphics Controller";
                case 0x0D26: return "Crystal Well Integrated Graphics Controller";
                case 0x0D2E: return "Crystal Well Integrated Graphics Controller";
                case 0x1602: return "Broadwell-U Integrated Graphics";
                case 0x1606: return "Broadwell-U Integrated Graphics";
                case 0x160A: return "Broadwell-U Integrated Graphics";
                case 0x160B: return "Broadwell-U Integrated Graphics";
                case 0x160D: return "Broadwell-U Integrated Graphics";
                case 0x160E: return "Broadwell-U Integrated Graphics";
                case 0x1612: return "HD Graphics 5600";
                case 0x1616: return "HD Graphics 5500";
                case 0x161A: return "HD Graphics P5700";
                case 0x161B: return "HD Graphics P5700";
                case 0x161D: return "HD Graphics P5700";
                case 0x161E: return "HD Graphics 5300";
                case 0x1622: return "Iris Pro Graphics 6200";
                case 0x1626: return "HD Graphics 6000";
                case 0x162A: return "Iris Pro Graphics P6300";
                case 0x162B: return "Iris Graphics 6100";
                case 0x162D: return "Iris Pro Graphics P6300";
                case 0x162E: return "Iris Graphics 6100";
                case 0x1902: return "HD Graphics 510";
                case 0x1906: return "HD Graphics 510";
                case 0x190A: return "HD Graphics P510";
                case 0x190B: return "HD Graphics 510";
                case 0x190E: return "HD Graphics 510";
                case 0x1912: return "HD Graphics 530";
                case 0x1916: return "HD Graphics 520";
                case 0x191A: return "HD Graphics P530";
                case 0x191B: return "HD Graphics 530";
                case 0x191D: return "HD Graphics P530";
                case 0x191E: return "HD Graphics 515";
                case 0x1921: return "HD Graphics 520";
                case 0x1923: return "HD Graphics 535";
                                case 0x1926: return "Iris Graphics 540";
                case 0x1927: return "Iris Graphics 550";
                case 0x192A: return "Iris Pro Graphics P555";
                case 0x192B: return "Iris Graphics 555";
                case 0x192D: return "Iris Pro Graphics P580";
                case 0x1932: return "Iris Pro Graphics 580";
                case 0x193A: return "Iris Pro Graphics P580";
                case 0x193B: return "Iris Pro Graphics 580";
                case 0x193D: return "Iris Pro Graphics P580";
                case 0x5902: return "HD Graphics 610";
                case 0x5906: return "HD Graphics 610";
                case 0x5908: return "HD Graphics 630";
                case 0x590A: return "HD Graphics P630";
                case 0x590B: return "HD Graphics 610";
                case 0x590E: return "HD Graphics 615";
                case 0x5912: return "HD Graphics 630";
                case 0x5916: return "HD Graphics 620";
                case 0x5917: return "UHD Graphics 620";
                case 0x591A: return "HD Graphics P630";
                case 0x591B: return "HD Graphics 630";
                case 0x591C: return "UHD Graphics 615";
                case 0x591D: return "HD Graphics P630";
                case 0x591E: return "HD Graphics 615";
                case 0x5921: return "HD Graphics 620";
                case 0x5923: return "HD Graphics 635";
                case 0x5926: return "Iris Plus Graphics 640";
                case 0x5927: return "Iris Plus Graphics 650";
                case 0x87C0: return "UHD Graphics 617";
                case 0x87CA: return "UHD Graphics 615";
                case 0x8A50: return "UHD Graphics (Gen11 Type 1)";
                case 0x8A51: return "UHD Graphics (Gen11 Type 1)";
                case 0x8A52: return "UHD Graphics (Gen11 Type 1)";
                case 0x8A53: return "UHD Graphics (Gen11 Type 1)";
                case 0x8A56: return "UHD Graphics (Gen11 Type 1)";
                case 0x8A57: return "UHD Graphics (Gen11 Type 1)";
                case 0x8A58: return "UHD Graphics (Gen11 Type 1)";
                case 0x8A59: return "UHD Graphics (Gen11 Type 1)";
                case 0x8A5A: return "UHD Graphics (Gen11 Type 1)";
                case 0x8A5B: return "UHD Graphics (Gen11 Type 1)";
                case 0x8A5C: return "UHD Graphics (Gen11 Type 1)";
                case 0x8A5D: return "UHD Graphics (Gen11 Type 1)";
                case 0x9A40: return "TigerLake-LP GT1 [UHD Graphics G1]";
                case 0x9A49: return "TigerLake-LP GT2 [Iris Xe Graphics]";
                case 0x9A60: return "TigerLake-H GT1 [UHD Graphics]";
                case 0x9A68: return "TigerLake-H GT1 [UHD Graphics]";
                case 0x9A70: return "TigerLake-H GT1 [UHD Graphics]";
                case 0x9A78: return "TigerLake-H GT2 [UHD Graphics]";
                case 0x4680: return "DG1 [Iris Xe MAX Graphics]";
                case 0x4682: return "DG1 [Iris Xe MAX Graphics]";
                case 0x4688: return "DG1 [Iris Xe MAX Graphics]";
                case 0x468A: return "DG1 [Iris Xe MAX Graphics]";
                case 0x468B: return "DG1 [Iris Xe MAX Graphics]";
                case 0x4690: return "DG1 [Iris Xe MAX Graphics]";
                case 0x4692: return "DG1 [Iris Xe MAX Graphics]";
                case 0x4693: return "DG1 [Iris Xe MAX Graphics]";
                case 0x56A0: return "DG2 [Arc A770M]";
                case 0x56A1: return "DG2 [Arc A750M]";
                case 0x56A2: return "DG2 [Arc A550M]";
                case 0x56A3: return "DG2 [Arc A370M]";
                case 0x56A4: return "DG2 [Arc A580]";
                case 0x56A5: return "DG2 [Arc A750]";
                case 0x56A6: return "DG2 [Arc A580]";
                case 0x5690: return "DG2 [Arc A770]";
                case 0x5691: return "DG2 [Arc A750]";
                case 0x5692: return "DG2 [Arc A580]";
                case 0x5693: return "DG2 [Arc A380]";
                case 0x5694: return "DG2 [Arc A580]";
                case 0x5695: return "DG2 [Arc A750]";
                case 0x5696: return "DG2 [Arc A580]";
                case 0x5697: return "DG2 [Arc A750]";
                default: return "Intel Graphics Controller";
            }
            
        case GPU_VENDOR_VMWARE:
            switch (device_id) {
                case 0x0405: return "SVGA II Adapter";
                case 0x0710: return "SVGA Adapter";
                default: return "VMware SVGA";
            }
            
        case GPU_VENDOR_VIRTUALBOX:
            switch (device_id) {
                case 0xBEEF: return "VirtualBox Graphics Adapter";
                default: return "VirtualBox Graphics";
            }
            
        case GPU_VENDOR_QEMU:
            switch (device_id) {
                case 0x1111: return "QEMU VGA";
                default: return "QEMU Graphics";
            }
            
        case GPU_VENDOR_CIRRUS:
            switch (device_id) {
                case 0x00A0: return "GD 5430/40 [Alpine]";
                case 0x00A2: return "GD 5432 [Alpine]";
                case 0x00A4: return "GD 5434-4 [Alpine]";
                case 0x00A8: return "GD 5434-8 [Alpine]";
                case 0x00AC: return "GD 5436 [Alpine]";
                case 0x00B0: return "GD 5440";
                case 0x00B8: return "GD 5446";
                case 0x00BC: return "GD 5480";
                case 0x00D0: return "GD 5462";
                case 0x00D2: return "GD 5462 [Laguna I]";
                case 0x00D4: return "GD 5464 [Laguna]";
                case 0x00D5: return "GD 5464 BD [Laguna]";
                case 0x00D6: return "GD 5465 [Laguna]";
                case 0x1200: return "GD 7542 [Nordic]";
                case 0x1202: return "GD 7543 [Viking]";
                case 0x1204: return "GD 7541 [Nordic Light]";
                default: return "Cirrus Logic Graphics";
            }
            
        case GPU_VENDOR_S3:
            switch (device_id) {
                case 0x5631: return "86c325 [ViRGE]";
                case 0x8811: return "86c764/765 [Trio32/64/64V+]";
                case 0x8812: return "86c764/765 [Trio32/64/64V+]";
                case 0x8813: return "86c764/765 [Trio32/64/64V+]";
                case 0x8814: return "86c767 [Trio 64UV+]";
                case 0x8815: return "86c765 [Trio64V2/DX or /GX]";
                case 0x883D: return "86c988 [ViRGE/VX]";
                case 0x8870: return "FireGL";
                case 0x8880: return "86c868 [Vision 868 VRAM] rev 0";
                case 0x8881: return "86c868 [Vision 868 VRAM] rev 1";
                case 0x8882: return "86c868 [Vision 868 VRAM] rev 2";
                case 0x8883: return "86c868 [Vision 868 VRAM] rev 3";
                case 0x88B0: return "86c928 [Vision 928 VRAM] rev 0";
                case 0x88B1: return "86c928 [Vision 928 VRAM] rev 1";
                case 0x88B2: return "86c928 [Vision 928 VRAM] rev 2";
                case 0x88B3: return "86c928 [Vision 928 VRAM] rev 3";
                case 0x88C0: return "86c864 [Vision 864 DRAM] rev 0";
                case 0x88C1: return "86c864 [Vision 864 DRAM] rev 1";
                case 0x88C2: return "86c864 [Vision 864-P DRAM] rev 2";
                case 0x88C3: return "86c864 [Vision 864-P DRAM] rev 3";
                case 0x88D0: return "86c964 [Vision 964 VRAM] rev 0";
                case 0x88D1: return "86c964 [Vision 964 VRAM] rev 1";
                case 0x88D2: return "86c964 [Vision 964-P VRAM] rev 2";
                case 0x88D3: return "86c964 [Vision 964-P VRAM] rev 3";
                case 0x88F0: return "86c968 [Vision 968 VRAM] rev 0";
                case 0x88F1: return "86c968 [Vision 968 VRAM] rev 1";
                case 0x88F2: return "86c968 [Vision 968 VRAM] rev 2";
                case 0x88F3: return "86c968 [Vision 968 VRAM] rev 3";
                case 0x8901: return "86c755 [Trio 64V2/DX]";
                case 0x8902: return "86c551";
                case 0x8903: return "86c551";
                case 0x8904: return "86c625";
                case 0x8905: return "86c864 [Vision 864]";
                case 0x8906: return "86c964 [Vision 964]";
                case 0x8907: return "86c732 [Trio 32]";
                case 0x8908: return "86c764 [Trio 64]";
                case 0x8909: return "86c765 [Trio 64V+]";
                case 0x890A: return "86c868 [Vision 868]";
                case 0x890B: return "86c928 [Vision 928]";
                case 0x890C: return "86c887 [ViRGE]";
                case 0x890D: return "86c889 [ViRGE]";
                case 0x890E: return "86c887 [ViRGE]";
                case 0x890F: return "86c889 [ViRGE]";
                case 0x8A01: return "86c375 [ViRGE/DX] or 86c385 [ViRGE/GX]";
                case 0x8A10: return "86c357 [ViRGE/GX2]";
                case 0x8A11: return "86c359 [ViRGE/GX2+]";
                case 0x8A12: return "86c359 [ViRGE/GX2+]";
                case 0x8A13: return "86c368 [Trio 3D/2X]";
                case 0x8A20: return "86c794 [Savage 3D]";
                case 0x8A21: return "86c390 [Savage 3D/MV]";
                case 0x8A22: return "86c391 [Savage 4]";
                case 0x8A23: return "86c394 [Savage 4]";
                case 0x8A25: return "86c397 [Savage 4]";
                case 0x8A26: return "86c397 [Savage 4]";
                case 0x8C00: return "86c270-294 [SavageIX-MV]";
                case 0x8C01: return "86c270-294 [SavageIX]";
                case 0x8C02: return "86c270-294 [SavageIX-MV]";
                case 0x8C03: return "86c270-294 [SavageIX]";
                case 0x8C10: return "86c270-294 [Savage/MX-MV]";
                case 0x8C11: return "86c270-294 [Savage/MX]";
                case 0x8C12: return "86c270-294 [Savage/MX-MV]";
                case 0x8C13: return "86c270-294 [Savage/MX]";
                case 0x8C22: return "SuperSavage MX/128";
                case 0x8C24: return "SuperSavage MX/64";
                case 0x8C26: return "SuperSavage MX/64C";
                case 0x8C2A: return "SuperSavage IX/128 SDR";
                case 0x8C2B: return "SuperSavage IX/128 DDR";
                case 0x8C2C: return "SuperSavage IX/64 SDR";
                case 0x8C2D: return "SuperSavage IX/64 DDR";
                case 0x8C2E: return "SuperSavage IX/C SDR";
                case 0x8C2F: return "SuperSavage IX/C DDR";
                case 0x9102: return "86c410 Savage 2000";
                default: return "S3 Graphics Card";
            }
            
        case GPU_VENDOR_MATROX:
            switch (device_id) {
                                case 0x0518: return "MGA-II [Athena]";
                case 0x0519: return "MGA 2064W [Millennium]";
                case 0x051A: return "MGA 1064SG [Mystique]";
                case 0x051B: return "MGA 2164W [Millennium II]";
                case 0x051F: return "MGA 2164W [Millennium II AGP]";
                case 0x0520: return "MGA G200";
                case 0x0521: return "MGA G200 AGP";
                case 0x0525: return "MGA G400/G450";
                case 0x2527: return "MGA G550 AGP";
                case 0x0532: return "MGA G200eV";
                case 0x0533: return "MGA G200WB";
                case 0x0534: return "G200eR2";
                case 0x0536: return "Integrated Matrox G200eW3 Graphics Controller";
                case 0x0538: return "MGA G200eH";
                case 0x1000: return "MGA G100 [Productiva]";
                case 0x1001: return "MGA G100 [Productiva] AGP";
                case 0x2007: return "MGA Mistral";
                default: return "Matrox Graphics Card";
            }
            
        case GPU_VENDOR_3DFX:
            switch (device_id) {
                case 0x0001: return "Voodoo";
                case 0x0002: return "Voodoo 2";
                case 0x0003: return "Voodoo Banshee";
                case 0x0004: return "Voodoo Banshee [Velocity 100]";
                case 0x0005: return "Voodoo 3";
                case 0x0007: return "Voodoo 4 / Voodoo 5";
                case 0x0009: return "Voodoo 5 AGP 5500/6000";
                default: return "3dfx Graphics Card";
            }
            
        default:
            return "Unknown Graphics Device";
    }
}

// Detect VGA display modes
void gpu_detect_vga_modes(gpu_device_t* gpu) {
    // Standard VGA modes
    gpu->mode_count = 0;
    
    // Mode 0x03: 80x25 text mode
    gpu->supported_modes[gpu->mode_count].width = 80;
    gpu->supported_modes[gpu->mode_count].height = 25;
    gpu->supported_modes[gpu->mode_count].bpp = 4;
    gpu->supported_modes[gpu->mode_count].refresh_rate = 70;
    gpu->supported_modes[gpu->mode_count].supported = 1;
    gpu->mode_count++;
    
    // Mode 0x12: 640x480x4
    gpu->supported_modes[gpu->mode_count].width = 640;
    gpu->supported_modes[gpu->mode_count].height = 480;
    gpu->supported_modes[gpu->mode_count].bpp = 4;
    gpu->supported_modes[gpu->mode_count].refresh_rate = 60;
    gpu->supported_modes[gpu->mode_count].supported = 1;
    gpu->mode_count++;
    
    // Mode 0x13: 320x200x8
    gpu->supported_modes[gpu->mode_count].width = 320;
    gpu->supported_modes[gpu->mode_count].height = 200;
    gpu->supported_modes[gpu->mode_count].bpp = 8;
    gpu->supported_modes[gpu->mode_count].refresh_rate = 70;
    gpu->supported_modes[gpu->mode_count].supported = 1;
    gpu->mode_count++;
    
    // VESA modes (if supported)
    // 640x480x8
    gpu->supported_modes[gpu->mode_count].width = 640;
    gpu->supported_modes[gpu->mode_count].height = 480;
    gpu->supported_modes[gpu->mode_count].bpp = 8;
    gpu->supported_modes[gpu->mode_count].refresh_rate = 60;
    gpu->supported_modes[gpu->mode_count].supported = 1;
    gpu->mode_count++;
    
    // 800x600x8
    gpu->supported_modes[gpu->mode_count].width = 800;
    gpu->supported_modes[gpu->mode_count].height = 600;
    gpu->supported_modes[gpu->mode_count].bpp = 8;
    gpu->supported_modes[gpu->mode_count].refresh_rate = 60;
    gpu->supported_modes[gpu->mode_count].supported = 1;
    gpu->mode_count++;
    
    // 1024x768x8
    gpu->supported_modes[gpu->mode_count].width = 1024;
    gpu->supported_modes[gpu->mode_count].height = 768;
    gpu->supported_modes[gpu->mode_count].bpp = 8;
    gpu->supported_modes[gpu->mode_count].refresh_rate = 60;
    gpu->supported_modes[gpu->mode_count].supported = 1;
    gpu->mode_count++;
    
    // Set current mode (assume text mode initially)
    gpu->current_width = 80;
    gpu->current_height = 25;
    gpu->current_bpp = 4;
    gpu->current_refresh = 70;
}

// Detect GPU memory size
uint32_t gpu_detect_memory_size(gpu_device_t* gpu) {
    // This is a simplified detection - real implementation would
    // need to read from GPU-specific registers
    
    switch (gpu->vendor_id) {
        case GPU_VENDOR_NVIDIA:
            // NVIDIA cards typically have dedicated memory
            if (gpu->device_id >= 0x1F00) return 8192; // RTX series
            if (gpu->device_id >= 0x1B80) return 8192; // GTX 10 series
            if (gpu->device_id >= 0x1380) return 2048; // GTX 700 series
            if (gpu->device_id >= 0x1180) return 2048; // GTX 600 series
            if (gpu->device_id >= 0x0640) return 1024; // GTX 400/500 series
            return 512; // Older cards
            
        case GPU_VENDOR_AMD:
            // AMD cards
            if (gpu->device_id >= 0x7300) return 16384; // RX 6000/7000 series
            if (gpu->device_id >= 0x67DF) return 8192;  // RX 400/500 series
            if (gpu->device_id >= 0x6798) return 3072;  // HD 7000 series
            if (gpu->device_id >= 0x6738) return 1024;  // HD 6000 series
            return 512; // Older cards
            
        case GPU_VENDOR_INTEL:
            // Intel integrated graphics - shared memory
            gpu->memory_type = GPU_MEM_SHARED;
            if (gpu->device_id >= 0x9A40) return 2048; // Tiger Lake
            if (gpu->device_id >= 0x8A50) return 1024; // Ice Lake
            if (gpu->device_id >= 0x5902) return 512;  // Kaby Lake
            if (gpu->device_id >= 0x1902) return 256;  // Skylake
            return 128; // Older integrated
            
        case GPU_VENDOR_VMWARE:
        case GPU_VENDOR_VIRTUALBOX:
        case GPU_VENDOR_QEMU:
            // Virtual GPUs
            gpu->memory_type = GPU_MEM_SHARED;
            return 128; // Typical virtual GPU memory
            
        default:
            return 64; // Default for unknown cards
    }
}

// Detect GPU capabilities
uint32_t gpu_detect_capabilities(gpu_device_t* gpu) {
    uint32_t caps = 0;
    
    // All GPUs support basic 2D
    caps |= GPU_CAP_2D;
    
    // VGA compatibility for most cards
    if (gpu->vendor_id != GPU_VENDOR_VMWARE) {
        caps |= GPU_CAP_VGA_COMPAT;
    }
    
    switch (gpu->vendor_id) {
        case GPU_VENDOR_NVIDIA:
            caps |= GPU_CAP_3D | GPU_CAP_HARDWARE_ACCEL | GPU_CAP_SHADER;
            caps |= GPU_CAP_DIRECTX | GPU_CAP_OPENGL;
            caps |= GPU_CAP_MULTI_MONITOR | GPU_CAP_HDMI | GPU_CAP_DISPLAYPORT;
            
            if (gpu->device_id >= 0x1F00) { // RTX series
                caps |= GPU_CAP_VULKAN | GPU_CAP_COMPUTE;
                caps |= GPU_CAP_VIDEO_DECODE | GPU_CAP_VIDEO_ENCODE;
            } else if (gpu->device_id >= 0x1180) { // GTX 600+
                caps |= GPU_CAP_VULKAN | GPU_CAP_COMPUTE;
                caps |= GPU_CAP_VIDEO_DECODE;
            } else if (gpu->device_id >= 0x0400) { // GTX 200+
                caps |= GPU_CAP_COMPUTE;
            }
            break;
            
        case GPU_VENDOR_AMD:
            caps |= GPU_CAP_3D | GPU_CAP_HARDWARE_ACCEL | GPU_CAP_SHADER;
            caps |= GPU_CAP_DIRECTX | GPU_CAP_OPENGL;
            caps |= GPU_CAP_MULTI_MONITOR | GPU_CAP_HDMI | GPU_CAP_DISPLAYPORT;
            
            if (gpu->device_id >= 0x7300) { // RX 6000/7000 series
                caps |= GPU_CAP_VULKAN | GPU_CAP_COMPUTE;
                caps |= GPU_CAP_VIDEO_DECODE | GPU_CAP_VIDEO_ENCODE;
            } else if (gpu->device_id >= 0x67DF) { // RX 400/500 series
                caps |= GPU_CAP_VULKAN | GPU_CAP_COMPUTE;
                caps |= GPU_CAP_VIDEO_DECODE;
            } else if (gpu->device_id >= 0x6798) { // HD 7000 series
                caps |= GPU_CAP_COMPUTE;
            }
            break;
            
        case GPU_VENDOR_INTEL:
            caps |= GPU_CAP_3D | GPU_CAP_HARDWARE_ACCEL;
            caps |= GPU_CAP_DIRECTX | GPU_CAP_OPENGL;
            caps |= GPU_CAP_MULTI_MONITOR | GPU_CAP_HDMI;
            
            if (gpu->device_id >= 0x8A50) { // Gen11+
                caps |= GPU_CAP_VULKAN | GPU_CAP_COMPUTE;
                caps |= GPU_CAP_VIDEO_DECODE | GPU_CAP_VIDEO_ENCODE;
                caps |= GPU_CAP_DISPLAYPORT;
            } else if (gpu->device_id >= 0x1902) { // Gen9+
                caps |= GPU_CAP_COMPUTE;
                caps |= GPU_CAP_VIDEO_DECODE;
            }
            break;
            
        case GPU_VENDOR_VMWARE:
        case GPU_VENDOR_VIRTUALBOX:
        case GPU_VENDOR_QEMU:
            caps |= GPU_CAP_3D; // Basic 3D acceleration in VMs
            break;
            
        default:
            // Basic capabilities for unknown cards
            break;
    }
    
    return caps;
}

// Initialize GPU subsystem
void gpu_init(void) {
    gpu_count = 0;
    
    // Clear all GPU device structures
    for (int i = 0; i < 8; i++) {
        gpu_devices[i].present = 0;
    }
    
    // Detect GPU devices
    gpu_detect_devices();
}

// Detect GPU devices via PCI
void gpu_detect_devices(void) {
    // Scan PCI bus for display controllers (class 0x03)
    for (int bus = 0; bus < 256; bus++) {
        for (int device = 0; device < 32; device++) {
            for (int function = 0; function < 8; function++) {
                uint16_t vendor_id = pci_config_read_word(bus, device, function, 0x00);
                
                if (vendor_id == 0xFFFF) continue; // No device
                
                uint8_t class_code = pci_config_read_byte(bus, device, function, 0x0B);
                uint8_t subclass = pci_config_read_byte(bus, device, function, 0x0A);
                
                // Check if it's a display controller (class 0x03)
                if (class_code == 0x03 && gpu_count < 8) {
                    gpu_device_t* gpu = &gpu_devices[gpu_count];
                    
                    gpu->present = 1;
                    gpu->primary = (gpu_count == 0) ? 1 : 0;
                    gpu->vendor_id = vendor_id;
                    gpu->device_id = pci_config_read_word(bus, device, function, 0x02);
                    gpu->revision = pci_config_read_byte(bus, device, function, 0x08);
                    gpu->class_code = class_code;
                    gpu->subclass = subclass;
                    gpu->prog_if = pci_config_read_byte(bus, device, function, 0x09);
                    gpu->interrupt_line = pci_config_read_byte(bus, device, function, 0x3C);
                    gpu->interrupt_pin = pci_config_read_byte(bus, device, function, 0x3D);
                    
                    // Read BAR addresses
                    for (int bar = 0; bar < 6; bar++) {
                        gpu->base_address[bar] = pci_config_read_dword(bus, device, function, 0x10 + (bar * 4));
                    }
                    
                    // Determine GPU type
                    if (subclass == 0x00) {
                        gpu->gpu_type = GPU_TYPE_VGA;
                    } else if (subclass == 0x01) {
                        gpu->gpu_type = GPU_TYPE_ACCELERATED;
                    } else {
                        gpu->gpu_type = GPU_TYPE_UNKNOWN;
                    }
                    
                    // Check for virtual GPUs
                    if (vendor_id == GPU_VENDOR_VMWARE || vendor_id == GPU_VENDOR_VIRTUALBOX || 
                        vendor_id == GPU_VENDOR_QEMU) {
                        gpu->gpu_type = GPU_TYPE_VIRTUAL;
                    }
                    
                    // Check for integrated GPUs
                    if (vendor_id == GPU_VENDOR_INTEL) {
                        gpu->gpu_type = GPU_TYPE_INTEGRATED;
                    }
                    
                                        // Set vendor and device names
                    strncpy(gpu->vendor_name, gpu_get_vendor_name(vendor_id), 31);
                    gpu->vendor_name[31] = '\0';
                    strncpy(gpu->device_name, gpu_get_device_name(vendor_id, gpu->device_id), 63);
                    gpu->device_name[63] = '\0';
                    
                    // Detect memory size and type
                    gpu->memory_size = gpu_detect_memory_size(gpu);
                    if (gpu->memory_type == 0) {
                        gpu->memory_type = (vendor_id == GPU_VENDOR_INTEL) ? GPU_MEM_SHARED : GPU_MEM_DEDICATED;
                    }
                    
                    // Detect capabilities
                    gpu->capabilities = gpu_detect_capabilities(gpu);
                    
                    // Detect display modes
                    gpu_detect_vga_modes(gpu);
                    
                    // Set driver version
                    strncpy(gpu->driver_version, "1.0.0", 15);
                    gpu->driver_version[15] = '\0';
                    
                    // Initialize performance counters (placeholder values)
                    gpu->core_clock = 0;
                    gpu->memory_clock = 0;
                    gpu->temperature = 0;
                    gpu->fan_speed = 0;
                    gpu->power_usage = 0;
                    
                    gpu_count++;
                }
            }
        }
    }
}

// Get number of detected GPU devices
int gpu_get_device_count(void) {
    return gpu_count;
}

// Get GPU device information
gpu_device_t* gpu_get_device_info(int device_index) {
    if (device_index < 0 || device_index >= gpu_count) {
        return NULL;
    }
    return &gpu_devices[device_index];
}

// Set display mode (placeholder implementation)
void gpu_set_display_mode(int device_index, uint16_t width, uint16_t height, uint8_t bpp) {
    if (device_index < 0 || device_index >= gpu_count) {
        return;
    }
    
    gpu_device_t* gpu = &gpu_devices[device_index];
    
    // Check if mode is supported
    for (int i = 0; i < gpu->mode_count; i++) {
        if (gpu->supported_modes[i].width == width &&
            gpu->supported_modes[i].height == height &&
            gpu->supported_modes[i].bpp == bpp &&
            gpu->supported_modes[i].supported) {
            
            gpu->current_width = width;
            gpu->current_height = height;
            gpu->current_bpp = bpp;
            gpu->current_refresh = gpu->supported_modes[i].refresh_rate;
            
            // Here you would implement the actual mode setting
            // This would involve writing to GPU registers
            break;
        }
    }
}

// Print detailed GPU device information
void gpu_print_device_info(int device_index) {
    if (device_index < 0 || device_index >= gpu_count) {
        terminal_writestring("Invalid GPU device index\n");
        return;
    }
    
    gpu_device_t* gpu = &gpu_devices[device_index];
    char buffer[32];
    
    terminal_writestring("GPU Device ");
    itoa(device_index, buffer, 10);
    terminal_writestring(buffer);
    if (gpu->primary) {
        terminal_writestring(" (Primary)");
    }
    terminal_writestring(":\n");
    
    // Vendor and device information
    terminal_writestring("  Vendor: ");
    terminal_writestring(gpu->vendor_name);
    terminal_writestring(" (ID: 0x");
    itoa(gpu->vendor_id, buffer, 16);
    terminal_writestring(buffer);
    terminal_writestring(")\n");
    
    terminal_writestring("  Device: ");
    terminal_writestring(gpu->device_name);
    terminal_writestring(" (ID: 0x");
    itoa(gpu->device_id, buffer, 16);
    terminal_writestring(buffer);
    terminal_writestring(")\n");
    
    terminal_writestring("  Revision: 0x");
    itoa(gpu->revision, buffer, 16);
    terminal_writestring(buffer);
    terminal_writestring("\n");
    
    // GPU type
    terminal_writestring("  Type: ");
    switch (gpu->gpu_type) {
        case GPU_TYPE_VGA:
            terminal_writestring("VGA Compatible");
            break;
        case GPU_TYPE_SVGA:
            terminal_writestring("Super VGA");
            break;
        case GPU_TYPE_ACCELERATED:
            terminal_writestring("Hardware Accelerated");
            break;
        case GPU_TYPE_INTEGRATED:
            terminal_writestring("Integrated Graphics");
            break;
        case GPU_TYPE_DISCRETE:
            terminal_writestring("Discrete Graphics Card");
            break;
        case GPU_TYPE_VIRTUAL:
            terminal_writestring("Virtual GPU");
            break;
        default:
            terminal_writestring("Unknown");
            break;
    }
    terminal_writestring("\n");
    
    // Memory information
    terminal_writestring("  Memory: ");
    itoa(gpu->memory_size, buffer, 10);
    terminal_writestring(buffer);
    terminal_writestring(" MB (");
    switch (gpu->memory_type) {
        case GPU_MEM_SHARED:
            terminal_writestring("Shared");
            break;
        case GPU_MEM_DEDICATED:
            terminal_writestring("Dedicated");
            break;
        case GPU_MEM_UNIFIED:
            terminal_writestring("Unified");
            break;
        default:
            terminal_writestring("Unknown");
            break;
    }
    terminal_writestring(")\n");
    
    // Current display mode
    terminal_writestring("  Current Mode: ");
    itoa(gpu->current_width, buffer, 10);
    terminal_writestring(buffer);
    terminal_writestring("x");
    itoa(gpu->current_height, buffer, 10);
    terminal_writestring(buffer);
    terminal_writestring("x");
    itoa(gpu->current_bpp, buffer, 10);
    terminal_writestring(buffer);
    terminal_writestring(" @ ");
    itoa(gpu->current_refresh, buffer, 10);
    terminal_writestring(buffer);
    terminal_writestring("Hz\n");
    
    // Capabilities
    terminal_writestring("  Capabilities: ");
    if (gpu->capabilities & GPU_CAP_2D) terminal_writestring("2D ");
    if (gpu->capabilities & GPU_CAP_3D) terminal_writestring("3D ");
    if (gpu->capabilities & GPU_CAP_HARDWARE_ACCEL) terminal_writestring("HW-Accel ");
    if (gpu->capabilities & GPU_CAP_SHADER) terminal_writestring("Shaders ");
    if (gpu->capabilities & GPU_CAP_DIRECTX) terminal_writestring("DirectX ");
    if (gpu->capabilities & GPU_CAP_OPENGL) terminal_writestring("OpenGL ");
    if (gpu->capabilities & GPU_CAP_VULKAN) terminal_writestring("Vulkan ");
    if (gpu->capabilities & GPU_CAP_COMPUTE) terminal_writestring("Compute ");
    if (gpu->capabilities & GPU_CAP_VIDEO_DECODE) terminal_writestring("Video-Dec ");
    if (gpu->capabilities & GPU_CAP_VIDEO_ENCODE) terminal_writestring("Video-Enc ");
    if (gpu->capabilities & GPU_CAP_MULTI_MONITOR) terminal_writestring("Multi-Mon ");
    if (gpu->capabilities & GPU_CAP_HDMI) terminal_writestring("HDMI ");
    if (gpu->capabilities & GPU_CAP_DISPLAYPORT) terminal_writestring("DisplayPort ");
    if (gpu->capabilities & GPU_CAP_VGA_COMPAT) terminal_writestring("VGA-Compat ");
    terminal_writestring("\n");
    
    // Base addresses
    terminal_writestring("  Base Addresses:\n");
    for (int i = 0; i < 6; i++) {
        if (gpu->base_address[i] != 0) {
            terminal_writestring("    BAR");
            itoa(i, buffer, 10);
            terminal_writestring(buffer);
            terminal_writestring(": 0x");
            itoa(gpu->base_address[i], buffer, 16);
            terminal_writestring(buffer);
            
            // Determine if it's memory or I/O
            if (gpu->base_address[i] & 1) {
                terminal_writestring(" (I/O)");
            } else {
                terminal_writestring(" (Memory");
                if (gpu->base_address[i] & 8) {
                    terminal_writestring(", Prefetchable");
                }
                terminal_writestring(")");
            }
            terminal_writestring("\n");
        }
    }
    
    // Interrupt information
    if (gpu->interrupt_line != 0xFF) {
        terminal_writestring("  Interrupt: Line ");
        itoa(gpu->interrupt_line, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring(", Pin ");
        itoa(gpu->interrupt_pin, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring("\n");
    }
    
    // Driver information
    terminal_writestring("  Driver Version: ");
    terminal_writestring(gpu->driver_version);
    terminal_writestring("\n");
    
    // Supported display modes
    terminal_writestring("  Supported Modes: ");
    itoa(gpu->mode_count, buffer, 10);
    terminal_writestring(buffer);
    terminal_writestring(" modes available\n");
    
    for (int i = 0; i < gpu->mode_count && i < 8; i++) { // Show first 8 modes
        terminal_writestring("    ");
        itoa(gpu->supported_modes[i].width, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring("x");
        itoa(gpu->supported_modes[i].height, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring("x");
        itoa(gpu->supported_modes[i].bpp, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring(" @ ");
        itoa(gpu->supported_modes[i].refresh_rate, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring("Hz");
        if (!gpu->supported_modes[i].supported) {
            terminal_writestring(" (Unsupported)");
        }
        terminal_writestring("\n");
    }
    
    if (gpu->mode_count > 8) {
        terminal_writestring("    ... and ");
        itoa(gpu->mode_count - 8, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring(" more modes\n");
    }
}
