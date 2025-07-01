import spidev
import RPi.GPIO as GPIO
import time
from PIL import Image, ImageDraw, ImageFont
from opencc import OpenCC

cc = OpenCC('s2t')  # Simplified to Traditional

WIDTH, HEIGHT = 128, 160
DC_PIN, RST_PIN = 25, 24

# Singleton SPI/GPIO initialization
spi = None
gpio_initialized = False

def ensure_spi_gpio():
    global spi, gpio_initialized
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)  # Always set mode
    if not gpio_initialized:
        GPIO.setup(DC_PIN, GPIO.OUT)
        GPIO.setup(RST_PIN, GPIO.OUT)
        spi = spidev.SpiDev()
        spi.open(0, 0)
        spi.max_speed_hz = 16000000
        spi.mode = 0b00
        gpio_initialized = True

def write_command(cmd):
    GPIO.output(DC_PIN, GPIO.LOW)
    spi.xfer([cmd])

def write_data(data):
    GPIO.output(DC_PIN, GPIO.HIGH)
    if data:
        spi.xfer(data)

def init_display():
    GPIO.output(RST_PIN, GPIO.HIGH)
    time.sleep(0.1)
    GPIO.output(RST_PIN, GPIO.LOW)
    time.sleep(0.1)
    GPIO.output(RST_PIN, GPIO.HIGH)
    time.sleep(0.1)
    init_seq = [
        (0x01, None), ("delay", 150),
        (0x11, None), ("delay", 255),
        (0x3A, [0x05]),
        #(0x36, [0x00]),
        (0x36, [0xC0]),
        (0x29, None), ("delay", 100),
    ]
    for cmd, arg in init_seq:
        if cmd == "delay":
            time.sleep(arg / 1000.0)
        else:
            write_command(cmd)
            if arg:
                write_data(arg)

def wrap_text(text, font, draw, max_width):
    lines, line = [], ""
    for char in text:
        if draw.textlength(line + char, font=font) <= max_width:
            line += char
        else:
            lines.append(line)
            line = char
    if line:
        lines.append(line)
    return lines

def display_text(original, translated):
    font = ImageFont.truetype("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", 18)
    img = Image.new("RGB", (WIDTH, HEIGHT), "black")
    draw = ImageDraw.Draw(img)
    y = 0
    for label, text, color in [("原文: ", original, "white"), ("翻譯: ", translated, "red")]:
        lines = wrap_text(label + text, font, draw, WIDTH)
        for line in lines:
            draw.text((0, y), line, font=font, fill=color)
            y += font.getbbox(line)[3] - font.getbbox(line)[1]
            if y >= HEIGHT:
                break

    buf = []
    pixels = img.load()
    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b = pixels[x, y]
            color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            buf.extend([color >> 8, color & 0xFF])

    write_command(0x2A)
    write_data([0x00, 0x00, 0x00, WIDTH - 1])
    write_command(0x2B)
    write_data([0x00, 0x00, 0x00, HEIGHT - 1])
    write_command(0x2C)
    for i in range(0, len(buf), 4096):
        write_data(buf[i:i + 4096])

display_initialized = False
def display(original, translated):
    global display_initialized
    ensure_spi_gpio()
    if not display_initialized:
        init_display()
        display_initialized = True
    translated_trad = cc.convert(translated)
    print(f"Displaying: {original} | {translated_trad}", flush=True)
    display_text(original, translated_trad)

if __name__ == "__main__":
    import sys
    original = sys.argv[1] if len(sys.argv) > 1 else "Original"
    translated = sys.argv[2] if len(sys.argv) > 2 else "Translated"
    display(original, translated)
    # Only close SPI/GPIO at the end of the script, not during C++ calls
    spi.close()
    GPIO.cleanup()