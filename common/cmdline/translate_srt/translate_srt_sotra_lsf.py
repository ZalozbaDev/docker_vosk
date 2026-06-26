#!/usr/bin/env python3
import sys
import pysrt
import requests

def translate_text(text, source_lang, target_lang, url="http://localhost:3000/translate"):
    """
    Sendet Text an den Übersetzungsdienst und gibt die Übersetzung zurück.
    """
    payload = {
        "text": text,
        "source_language": source_lang,
        "target_language": target_lang
    }
    headers = {"Content-Type": "application/json"}
    response = requests.post(url, json=payload, headers=headers)
    response.raise_for_status()
    return response.json().get("translation", text)

def main():
    if len(sys.argv) != 6:
        print(f"Usage: {sys.argv[0]} <input.srt> <output.srt> <source_lang> <target_lang> <url>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    source_lang = sys.argv[3]
    target_lang = sys.argv[4]
    url = sys.argv[5]

    subs = pysrt.open(input_file)

    for sub in subs:
        # Übersetze den Text der Untertitelzeile
        translated_text = translate_text(sub.text, source_lang, target_lang, url)
        sub.text = translated_text

    subs.save(output_file, encoding="utf-8")

if __name__ == "__main__":
    main()
