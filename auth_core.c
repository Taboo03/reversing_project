#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* =========================================================================
 * 플랫폼별 헤더 및 구현 (Windows / Linux / POSIX)
 * ========================================================================= */
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #define PLATFORM_WINDOWS 1
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <time.h>
    #if defined(__APPLE__)
        #include <sys/types.h>
        #include <sys/sysctl.h>
    #endif
    #define PLATFORM_POSIX 1
#endif

/* -------------------------------------------------------------------------
 * 1. 고정밀 타이머 (밀리초 단위 반환)
 * ------------------------------------------------------------------------- */
static double get_elapsed_ms(void) {
#if defined(PLATFORM_WINDOWS)
    static LARGE_INTEGER freq;
    static int initialized = 0;
    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = 1;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)(now.QuadPart * 1000.0) / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)(ts.tv_sec * 1000.0) + ((double)ts.tv_nsec / 1000000.0);
#endif
}

/* -------------------------------------------------------------------------
 * 2. 디버거 존재 감지 (IsDebuggerPresent 계열)
 * ------------------------------------------------------------------------- */
static int check_debugger(void) {
#if defined(PLATFORM_WINDOWS)
    /* Windows: PEB BeingDebugged 플래그 확인 API */
    return IsDebuggerPresent() != 0;

#elif defined(__APPLE__)
    /* macOS: sysctl P_TRACED 확인 */
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
    struct kinfo_proc info;
    size_t size = sizeof(info);
    memset(&info, 0, size);
    if (sysctl(mib, 4, &info, &size, NULL, 0) == 0) {
        return (info.kp_proc.p_flag & P_TRACED) != 0;
    }
    return 0;

#else
    /* Linux: /proc/self/status의 TracerPid 확인 */
    int fd = open("/proc/self/status", O_RDONLY);
    if (fd < 0) return 0;

    char buf[1024];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';

    char *tracer = strstr(buf, "TracerPid:");
    if (tracer) {
        int pid = 0;
        if (sscanf(tracer, "TracerPid:\t%d", &pid) == 1) {
            return (pid != 0);
        }
    }
    return 0;
#endif
}

/* -------------------------------------------------------------------------
 * 3. 난독화된 암호 데이터 테이블 (실제 패스워드 복호화용)
 * ------------------------------------------------------------------------- */
static const uint8_t ENC_SECRET[] = {
    0x15, 0x21, 0x20, 0x3c, 0x1f, 0x37, 0x2b, 0x0b,
    0x66, 0x64, 0x66, 0x62, 0x0b, 0x27, 0x1a, 0x64,
    0x64, 0x24, 0x0b, 0x39, 0x31, 0x39, 0x34, 0x3f, 0x75
};
static const size_t ENC_SECRET_LEN = sizeof(ENC_SECRET);

/* 컴파일러 최적화(Dead Store Elimination)로 인한 memset 생략 방지 */
static void secure_wipe(volatile void *buf, size_t len) {
    volatile uint8_t *p = (volatile uint8_t *)buf;
    while (len--) {
        *p++ = 0;
    }
}

/* -------------------------------------------------------------------------
 * 4. 핵심 검증 엔진 (Timing Trap & Volatile Memory)
 * ------------------------------------------------------------------------- */
static int verify_auth_token(const char *user_input) {
    int is_tampered = 0;

    /* [트랩 1] 디버거 감지 */
    if (check_debugger()) {
        is_tampered = 1;
    }

    /* [타이밍 측정 시작] */
    double t_start = get_elapsed_ms();

    /* 런타임에 메모리(스택) 상에서만 잠깐 복호화되는 휘발성 버퍼 */
    volatile char volatile_key[64];
    memset((void *)volatile_key, 0, sizeof(volatile_key));

    for (size_t i = 0; i < ENC_SECRET_LEN; i++) {
        /* 디버거 탐지 시 쓰레기값 생성, 정상일 때만 유효 키 생성 */
        uint8_t mask = is_tampered ? 0xFF : (0x54 ^ (uint8_t)(i * 3));
        volatile_key[i] = (char)(ENC_SECRET[i] ^ mask);
    }
    volatile_key[ENC_SECRET_LEN] = '\0';

    /* [타이밍 측정 종료] */
    double t_end = get_elapsed_ms();
    double delta_ms = t_end - t_start;

    /* [트랩 2] 지연 시간 탐지
     * 정상 연산은 0.01ms 미만. 사용자가 브레이크포인트를 걸고 멈칫하면 즉시 50ms 초과!
     */
    if (delta_ms > 50.0) {
        /* 타이밍 트랩 발동: 메모리를 즉시 가비지값으로 오염시킴 */
        for (size_t i = 0; i < sizeof(volatile_key); i++) {
            volatile_key[i] = (char)(rand() % 255);
        }
    }

    /* 사용자가 입력한 패스워드와 비교 */
    int is_match = (strcmp(user_input, (const char *)volatile_key) == 0);

    /* [핵심] 비교 완료 즉시 메모리에서 완전 파괴 */
    secure_wipe(volatile_key, sizeof(volatile_key));

    return is_match;
}

/* -------------------------------------------------------------------------
 * 메인 엔트리포인트
 * ------------------------------------------------------------------------- */
int main(void) {
    char input[128];

    printf("========================================\n");
    printf("   SECURE ACCESS VERIFICATION SYSTEM    \n");
    printf("========================================\n");
    printf("[*] Enter authorization key: ");
    fflush(stdout);

    if (!fgets(input, sizeof(input), stdin)) {
        return 1;
    }

    /* 개행 문자 제거 */
    input[strcspn(input, "\r\n")] = '\0';

    if (verify_auth_token(input)) {
        printf("\n[+] [SUCCESS] Authorization granted. Welcome, Admin.\n");
        return 0;
    } else {
        printf("\n[-] [DENIED] Invalid key or security violation detected.\n");
        return 1;
    }
}