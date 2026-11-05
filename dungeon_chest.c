#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <sys/ptrace.h>
#endif

// 암호화된 플래그 (FLAG{dungeon_treasure_unlocked!} ^ 0x5A)
static const unsigned char ENCRYPTED_KEY[] = {
    0x1c, 0x16, 0x1b, 0x1d, 0x21, 0x3e, 0x2f, 0x34, 0x3d, 0x3f, 0x35, 0x34,
    0x05, 0x2e, 0x28, 0x3f, 0x3b, 0x29, 0x2f, 0x28, 0x3f, 0x05, 0x2f, 0x34,
    0x36, 0x35, 0x39, 0x31, 0x3f, 0x3e, 0x7b, 0x27
};
#define KEY_LEN sizeof(ENCRYPTED_KEY)
#define XOR_KEY 0x5A

/*
 * [안티리버싱 기법] 디버거 탐지 함수
 * - macOS: sysctl을 통해 프로세스의 P_TRACED 플래그 검사
 * - Linux: /proc/self/status 에서 TracerPid 검사
 */
int is_debugger_present(void) {
#if defined(__APPLE__)
    int mib[4];
    struct kinfo_proc info;
    size_t size = sizeof(info);

    info.kp_proc.p_flag = 0;
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    if (sysctl(mib, 4, &info, &size, NULL, 0) == 0) {
        if ((info.kp_proc.p_flag & P_TRACED) != 0) {
            return 1; // 디버거 감지됨
        }
    }
    return 0;
#elif defined(__linux__)
    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp) return 0;

    char line[128];
    int tracer_pid = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "TracerPid:", 10) == 0) {
            tracer_pid = atoi(&line[10]);
            break;
        }
    }
    fclose(fp);
    return (tracer_pid != 0);
#else
    return 0;
#endif
}

void print_banner(void) {
    puts("=====================================================");
    puts("          🗡️  던전의 비밀: 마왕의 보물상자  🛡️          ");
    puts("=====================================================");
    puts("       .----------------------------------.          ");
    puts("      /|       ___________________        |\\         ");
    puts("     / |      | [?] [?] [?] [?] [?] |     | \\        ");
    puts("    *--+------|___________________|-------+--*       ");
    puts("    |  |      |                   |       |  |       ");
    puts("    |  |      |       [LOCK]      |       |  |       ");
    puts("    |  |______|___________________|_______|  |       ");
    puts("    *----------------------------------------*       ");
    puts("  어두운 던전 한가운데 거대한 고대 보물상자가 놓여 있습니다.");
    puts("  상자를 열기 위해 고대 주문(암호)을 입력해야 합니다...\n");
}

int main(void) {
    print_banner();

    // 1. 안티리버싱 검사: 디버거 감지
    printf("[*] 상자의 고대 결계를 확인하는 중입니다...\n");
    sleep(1);

    if (is_debugger_present()) {
        puts("\n[!] 👁️  사악한 시선이 당신의 영혼(디버거)을 꿰뚫어 보았습니다!");
        puts("[!] ⚠️  경고: 디버거가 감지되어 보물상자가 즉시 잠깁니다.");
        puts("[!] 상자가 뾰족한 이빨을 드러내며 미믹으로 변해 도망칩니다!\n");
        return 1;
    }

    puts("[+] 결계가 안전합니다. 상자가 당신의 목소리를 기다립니다.");

    // 2. 주문(패스워드) 입력
    char input[128] = {0};
    printf("\n[?] 보물상자를 열 주문을 외우세요: ");
    if (!fgets(input, sizeof(input), stdin)) {
        puts("[-] 입력을 읽을 수 없습니다.");
        return 1;
    }

    // 개행 문자 제거
    input[strcspn(input, "\r\n")] = '\0';

    // 3. 길이 검사
    size_t input_len = strlen(input);
    if (input_len != KEY_LEN) {
        puts("\n[-] 아무 일도 일어나지 않았습니다. 주문의 길이가 맞지 않는 것 같습니다...");
        return 1;
    }

    // 4. 플래그(주문) 검증: 고정 키 XOR 비교
    int success = 1;
    for (size_t i = 0; i < KEY_LEN; i++) {
        if ((unsigned char)(input[i] ^ XOR_KEY) != ENCRYPTED_KEY[i]) {
            success = 0;
            break;
        }
    }

    if (success) {
        puts("\n=====================================================");
        puts(" 🎉 [쿠구구궁...] 육중한 상자의 뚜껑이 천천히 열립니다!   ");
        puts("=====================================================");
        puts("       .----------------------------------.          ");
        puts("      /  \\       ✨  ✨  ✨  ✨         /  \\         ");
        puts("     /    \\___________________________/    \\         ");
        puts("    *------+-------------------------+------*        ");
        puts("    |      |                         |      |        ");
        puts("    |      |  👑 LEGENDARY REWARD 👑 |      |        ");
        puts("    |      |_________________________|      |        ");
        puts("    *---------------------------------------*        ");
        printf("\n 축하합니다! 마왕의 비보를 손에 넣었습니다!\n");
        printf(" 획득한 플래그: %s\n\n", input);
    } else {
        puts("\n[-] 치이익... 상자에서 보라색 연기가 뿜어져 나오며 주문이 튕겨 나갔습니다.");
        puts("[-] 주문이 틀렸습니다. 다시 시도하세요.\n");
    }

    return 0;
}
