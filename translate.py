import sys
from argostranslate import translate

def main():
    if len(sys.argv) != 3:
        print("Usage: translate.py input.txt output.txt")
        return

    input_file, output_file = sys.argv[1], sys.argv[2]

    with open(input_file, "r", encoding="utf-8") as f:
        text = f.read().strip()

    installed = translate.get_installed_languages()
    from_lang = next((x for x in installed if x.code == "en"), None)
    to_lang = next((x for x in installed if x.code == "zh"), None)

    if not from_lang or not to_lang:
        print("Languages not installed")
        return

    translation_fn = from_lang.get_translation(to_lang)
    translated_text = translation_fn.translate(text)

    with open(output_file, "w", encoding="utf-8") as f:
        f.write(translated_text)

if __name__ == "__main__":
    main()
