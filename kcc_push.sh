#!/bin/bash
# Kinetrix Fleet Push Tool
# Push compiled firmware to all OTA-enabled robots on the local network
#
# Usage:
#   ./kcc_push.sh robot.kx --target esp32 [--password kinetrix123]
#
# Prerequisites (ESP32):
#   pip install esptool
#   arduino-cli must be installed for compilation
#
# How it works:
#   1. Compiles the .kx file for the target platform
#   2. Discovers OTA-enabled robots on the local network
#   3. Pushes the compiled firmware to all robots

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KCC="$SCRIPT_DIR/kcc"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# Parse arguments
KX_FILE=""
TARGET="esp32"
PASSWORD=""
OUTPUT=""
RPI_USER="pi"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --target|-t)  TARGET="$2"; shift 2 ;;
        --password|-p) PASSWORD="$2"; shift 2 ;;
        --output|-o)  OUTPUT="$2"; shift 2 ;;
        --user|-u)    RPI_USER="$2"; shift 2 ;;
        --help|-h)
            echo "Kinetrix Fleet Push Tool"
            echo "========================"
            echo "Usage: $0 <source.kx> [--target <target>] [--password <pass>] [--user <ssh_user>]"
            echo ""
            echo "Targets: esp32 (default), rpi, pico"
            echo ""
            echo "Options:"
            echo "  --user, -u    SSH username for RPi targets (default: pi)"
            echo ""
            echo "Examples:"
            echo "  $0 robot.kx                          # Push to all ESP32s"
            echo "  $0 robot.kx --target esp32 -p secret # With OTA password"
            echo "  $0 robot.kx --target rpi --user soham # Push to all RPis via SSH"
            exit 0 ;;
        *)
            if [[ -z "$KX_FILE" ]]; then
                KX_FILE="$1"
            fi
            shift ;;
    esac
done

if [[ -z "$KX_FILE" ]]; then
    echo -e "${RED}Error: No source file specified${NC}"
    echo "Usage: $0 <source.kx> [--target <target>] [--password <pass>]"
    exit 1
fi

if [[ ! -f "$KX_FILE" ]]; then
    echo -e "${RED}Error: File '$KX_FILE' not found${NC}"
    exit 1
fi

echo -e "${CYAN}╔════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║   Kinetrix Fleet Push — OTA Update     ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════╝${NC}"
echo ""

# Step 1: Compile
EXT=".cpp"
case "$TARGET" in
    esp32)   EXT=".cpp" ;;
    rpi)     EXT=".py" ;;
    pico)    EXT=".py" ;;
    *)       echo -e "${RED}Unsupported target: $TARGET${NC}"; exit 1 ;;
esac

if [[ -z "$OUTPUT" ]]; then
    OUTPUT="/tmp/kinetrix_ota_build$(echo $EXT)"
fi

echo -e "${YELLOW}Step 1: Compiling $KX_FILE → $TARGET${NC}"
"$KCC" "$KX_FILE" --target "$TARGET" -o "$OUTPUT" 2>&1
if [[ $? -ne 0 ]]; then
    echo -e "${RED}Compilation failed!${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Compiled successfully${NC}"
echo ""

# Step 2: Discover robots
echo -e "${YELLOW}Step 2: Discovering robots on local network...${NC}"

ROBOTS=()

if [[ "$TARGET" == "esp32" ]]; then
    # Use dns-sd / avahi-browse to find ArduinoOTA devices
    if command -v dns-sd &>/dev/null; then
        # macOS
        echo "  Scanning with dns-sd (5 seconds)..."
        SCAN=$(timeout 5 dns-sd -B _arduino._tcp local 2>/dev/null || true)
        while IFS= read -r line; do
            name=$(echo "$line" | awk '{print $NF}')
            if [[ -n "$name" && "$name" != "Instance" ]]; then
                # Resolve IP
                ip=$(timeout 3 dns-sd -L "$name" _arduino._tcp local 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' | head -1)
                if [[ -n "$ip" ]]; then
                    ROBOTS+=("$name:$ip")
                fi
            fi
        done <<< "$SCAN"
    elif command -v avahi-browse &>/dev/null; then
        # Linux
        echo "  Scanning with avahi-browse (5 seconds)..."
        SCAN=$(timeout 5 avahi-browse -rp _arduino._tcp 2>/dev/null | grep '=;' || true)
        while IFS=';' read -r _ _ _ name _ _ addr _ _; do
            if [[ -n "$addr" ]]; then
                ROBOTS+=("$name:$addr")
            fi
        done <<< "$SCAN"
    fi
elif [[ "$TARGET" == "rpi" || "$TARGET" == "pico" ]]; then
    # Scan for Kinetrix mDNS services
    if command -v dns-sd &>/dev/null; then
        echo "  Scanning with dns-sd for _kinetrix._tcp (5 seconds)..."
        SCAN=$(timeout 5 dns-sd -B _kinetrix._tcp local 2>/dev/null || true)
    elif command -v avahi-browse &>/dev/null; then
        echo "  Scanning with avahi-browse (5 seconds)..."
        SCAN=$(timeout 5 avahi-browse -rp _kinetrix._tcp 2>/dev/null || true)
    fi
fi

echo ""

if [[ ${#ROBOTS[@]} -eq 0 ]]; then
    echo -e "${YELLOW}No robots discovered automatically.${NC}"
    echo ""
    echo "You can manually specify robot IPs. Enter IPs (comma-separated) or press Enter to skip:"
    read -r MANUAL_IPS
    if [[ -n "$MANUAL_IPS" ]]; then
        IFS=',' read -ra IPS <<< "$MANUAL_IPS"
        for ip in "${IPS[@]}"; do
            ip=$(echo "$ip" | xargs)  # trim whitespace
            ROBOTS+=("manual:$ip")
        done
    fi
fi

if [[ ${#ROBOTS[@]} -eq 0 ]]; then
    echo -e "${RED}No robots to update. Exiting.${NC}"
    echo ""
    echo "Compiled output is available at: $OUTPUT"
    echo ""
    echo "Manual upload commands:"
    case "$TARGET" in
        esp32)
            echo "  Arduino IDE: Open $OUTPUT → Upload"
            echo "  espota.py:   python3 espota.py -i <ROBOT_IP> -p 3232 -f <firmware.bin>"
            ;;
        rpi)
            echo "  scp $OUTPUT <user>@<ROBOT_IP>:~/robot.py && ssh <user>@<ROBOT_IP> 'python3 ~/robot.py'"
            ;;
        pico)
            echo "  mpremote connect <ROBOT_IP> cp $OUTPUT :main.py"
            ;;
    esac
    exit 0
fi

echo -e "${GREEN}Found ${#ROBOTS[@]} robot(s):${NC}"
for r in "${ROBOTS[@]}"; do
    name="${r%%:*}"
    ip="${r##*:}"
    echo -e "  ${CYAN}→${NC} $name ($ip)"
done
echo ""

# Step 3: Push to all robots
echo -e "${YELLOW}Step 3: Pushing update to ${#ROBOTS[@]} robot(s)...${NC}"
PUSH_OK=0
PUSH_FAIL=0

for r in "${ROBOTS[@]}"; do
    name="${r%%:*}"
    ip="${r##*:}"
    
    echo -n "  $name ($ip): "
    
    case "$TARGET" in
        esp32)
            # Use espota.py for ArduinoOTA push
            if command -v espota.py &>/dev/null; then
                CMD=(espota.py -i "$ip" -p 3232)
                if [[ -n "$PASSWORD" ]]; then CMD+=(-a "$PASSWORD"); fi
                CMD+=(-f "$OUTPUT")
                if "${CMD[@]}" 2>/dev/null; then
                    echo -e "${GREEN}✓${NC}"
                    PUSH_OK=$((PUSH_OK + 1))
                else
                    echo -e "${RED}✗${NC}"
                    PUSH_FAIL=$((PUSH_FAIL + 1))
                fi
            else
                echo -e "${YELLOW}espota.py not found, skipping${NC}"
                PUSH_FAIL=$((PUSH_FAIL + 1))
            fi
            ;;
        rpi)
            # Use SCP + SSH (username configurable via --user flag)
            if scp -q "$OUTPUT" "${RPI_USER}@${ip}:~/kinetrix_update.py" 2>/dev/null; then
                ssh -q "${RPI_USER}@${ip}" "pkill -f kinetrix_update.py 2>/dev/null; nohup python3 ~/kinetrix_update.py &>/dev/null &" 2>/dev/null
                echo -e "${GREEN}✓${NC}"
                PUSH_OK=$((PUSH_OK + 1))
            else
                echo -e "${RED}✗${NC}"
                PUSH_FAIL=$((PUSH_FAIL + 1))
            fi
            ;;
        pico)
            # Use mpremote for Pico W
            if command -v mpremote &>/dev/null; then
                if mpremote connect "$ip" cp "$OUTPUT" :main.py 2>/dev/null; then
                    mpremote connect "$ip" reset 2>/dev/null
                    echo -e "${GREEN}✓${NC}"
                    PUSH_OK=$((PUSH_OK + 1))
                else
                    echo -e "${RED}✗${NC}"
                    PUSH_FAIL=$((PUSH_FAIL + 1))
                fi
            else
                echo -e "${YELLOW}mpremote not found, skipping${NC}"
                PUSH_FAIL=$((PUSH_FAIL + 1))
            fi
            ;;
    esac
done

echo ""
echo -e "══════════════════════════════════════════"
echo -e "  ${GREEN}Updated: $PUSH_OK${NC}  |  ${RED}Failed: $PUSH_FAIL${NC}"
echo -e "══════════════════════════════════════════"

if [[ $PUSH_FAIL -gt 0 ]]; then
    exit 1
else
    echo -e "\n${GREEN}Fleet update complete! ✓${NC}"
    exit 0
fi
