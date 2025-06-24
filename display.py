import time
import os

from your_display_module import display_text  # Your existing display_text function

def main():
    last_text = ""
    while True:
        if os.path.exists("translated.txt"):
            with open("translated.txt", "r", encoding="utf-8") as f:
                text = f.read().strip()
            if text != last_text and text != "":
                last_text = text
                display_text(text)
        time.sleep(0.5)

if __name__ == "__main__":
    main()
