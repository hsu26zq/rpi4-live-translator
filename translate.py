# argos_translate_embed.py
from argostranslate import package, translate

def translate_text(text, from_lang="en", to_lang="zh"):
    available_packages = package.get_available_packages()
    installed_languages = translate.get_installed_languages()

    from_lang_obj = None
    to_lang_obj = None

    for lang in installed_languages:
        if lang.code == from_lang:
            from_lang_obj = lang
        if lang.code == to_lang:
            to_lang_obj = lang

    if not from_lang_obj or not to_lang_obj:
        # try install package if missing
        for pkg in available_packages:
            if pkg.from_code == from_lang and pkg.to_code == to_lang:
                pkg.install()
                installed_languages = translate.get_installed_languages()
                for lang in installed_languages:
                    if lang.code == from_lang:
                        from_lang_obj = lang
                    if lang.code == to_lang:
                        to_lang_obj = lang
                break

    if not from_lang_obj or not to_lang_obj:
        return "ERROR: Language package not found"

    translation = from_lang_obj.get_translation(to_lang_obj)
    return translation.translate(text)
