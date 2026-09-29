import math
from pathlib import Path

import kenlm
from tokenizers import Tokenizer


def _wordpiece_lm_text(tokenizer, text):
    text = " ".join(str(text).replace("#", " ").upper().split())
    tokens = tokenizer.encode(text, add_special_tokens=False).tokens
    words = []
    for token in tokens:
        if token.startswith("##") and words:
            words[-1].append(token[2:].upper())
        else:
            words.append([token.removeprefix("##").upper()])

    output = []
    for pieces in words:
        repaired = []
        for piece in pieces:
            if repaired and (
                (repaired[-1].endswith("C") and piece.startswith("H"))
                or (repaired[-1].endswith("D") and piece.startswith("Ź"))
            ):
                repaired[-1] += piece[0]
                piece = piece[1:]
            if piece:
                repaired.append(piece)
        output.extend("# #".join(repaired).split())
    return " ".join(output)


class SubwordLMRescorer:
    def __init__(self, lm_path, tokenizer_path, alpha=1.0, beta=0.0):
        self.lm = kenlm.Model(str(Path(lm_path)))
        self.tokenizer = Tokenizer.from_file(str(Path(tokenizer_path)))
        self.alpha = float(alpha)
        self.beta = float(beta)

    def lm_logp(self, text):
        encoded = _wordpiece_lm_text(self.tokenizer, text)
        return self.lm.score(encoded, bos=True, eos=True) * math.log(10.0)

    def rescore(self, hypotheses):
        if not hypotheses:
            return {"text": "", "acoustic_logp": float("-inf"), "lm_logp": float("-inf"), "score": float("-inf")}
        rescored = []
        for hypothesis in hypotheses:
            item = dict(hypothesis)
            item["lm_logp"] = self.lm_logp(item["text"])
            item["word_count"] = len(item["text"].split())
            item["score"] = (
                float(item["acoustic_logp"])
                + self.alpha * item["lm_logp"]
                + self.beta * item["word_count"]
            )
            rescored.append(item)
        return max(rescored, key=lambda item: item["score"])
