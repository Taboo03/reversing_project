# 🗡️ 던전의 비밀: 마왕의 보물상자 (Dungeon Chest)

직접 풀며 리버싱과 안티리버싱의 기본 개념을 익힐 수 있는 **입문자 친화적 리버싱 챌린지**입니다.

---

## 📌 1. 문제 개요
* **장르**: 텍스트 RPG 콘솔 어드벤처
* **시나리오**:
  던전 깊은 곳에서 발견한 고대 마왕의 보물상자. 상자를 열기 위해서는 올바른 주문(Password)을 입력해야 합니다.
  하지만 상자에는 마법 결계(안티리버싱)가 걸려 있어, 디버거로 감시하려 하면 상자가 미믹(괴물)으로 변해 도망칩니다!
* **목표**: 안티디버깅을 우회하거나 정적 분석을 통해 올바른 주문(FLAG)을 찾아 상자를 여는 것.

---

## 🛠️ 2. 컴파일 및 실행 방법

### 컴파일
터미널에서 아래 명령어 하나로 즉시 빌드됩니다:
```bash
make
```

### 일반 실행
```bash
./dungeon_chest
```

### 정답 입력 테스트
```bash
echo "FLAG{dungeon_treasure_unlocked!}" | ./dungeon_chest
```

---

## 🛡️ 3. 적용된 안티리버싱 기법

### 🔍 디버거 탐지 (`is_debugger_present`)
* **macOS**: `sysctl` 커널 API를 통해 현재 프로세스의 `p_flag`에 `P_TRACED` 플래그가 세팅되어 있는지 확인합니다.
* **Linux**: `/proc/self/status` 파일의 `TracerPid` 항목을 확인하여 디버거 프로세스가 붙어 있는지 검사합니다.

#### 디버거 실행 시 출력 예시:
```bash
lldb ./dungeon_chest
(lldb) run
...
[*] 상자의 고대 결계를 확인하는 중입니다...

[!] 👁️  사악한 시선이 당신의 영혼(디버거)을꿰뚫어 보았습니다!
[!] ⚠️  경고: 디버거가 감지되어 보물상자가 즉시 잠깁니다.
[!] 상자가 뾰족한 이빨을 드러내며 미믹으로 변해 도망칩니다!
```

---

## 💡 4. 풀이 가이드 (Write-up)

이 문제는 크게 **3가지 방법**으로 손쉽게 풀 수 있습니다.

### 방법 1. 정적 분석으로 바로 풀기 (가장 추천 ⭐)
디버거를 켜지 않고 Ghidra, IDA, Cutter 또는 Hex Fiend로 바이너리를 봅니다.
1. `main` 함수에서 `ENCRYPTED_KEY` 배열(32바이트)과 `XOR_KEY (0x5A)`를 확인합니다.
2. 각 바이트를 `0x5A`와 XOR 연산하면 즉시 플래그가 나옵니다.
   ```bash
   python3 solve.py
   # 출력: FLAG{dungeon_treasure_unlocked!}
   ```

### 방법 2. LLDB에서 안티디버깅 우회하기 (동적 디버깅)
LLDB로 실행하더라도 함수 반환값을 조작하여 우회할 수 있습니다.
```bash
lldb ./dungeon_chest
(lldb) b is_debugger_present
(lldb) run
(lldb) finish
(lldb) reg write x0 0     # 리턴값을 0(디버거 없음)으로 강제 변조
(lldb) continue
```
결계가 통과되고 주문 입력창이 정상적으로 나타납니다!

### 방법 3. 바이너리 패치
디스어셈블러에서 `is_debugger_present()` 호출 직후의 조건 분기 명령(예: `cbnz` 또는 `jne`)을 `nop`이나 반대 분기로 패치하면 디버거 유무에 상관없이 통과됩니다.

---

## 🚩 최종 플래그
`FLAG{dungeon_treasure_unlocked!}`
