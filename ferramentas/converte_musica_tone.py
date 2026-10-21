"""
Converte um sketch Arduino feito de chamadas tone(pino, frequencia, duracao) + delay(ms)
para a tabela de notas do Radar (musica 6).

Uso (no Prompt de Comando, dentro da pasta "projeto circuitos logicos"):
    python ferramentas\converte_musica_tone.py caminho\do\arquivo.ino

Gera Radar\Musica6.h. Com esse arquivo na pasta do sketch, o Radar ganha a
musica 6 (tecla 6 no modo musica). Sem ele, o programa continua com as 5 musicas.
Compile e grave de novo no Arduino depois de gerar.

Como converte: cada tone(...) vira uma nota; o delay(...) logo depois define o tempo
total da nota (se nao houver, vale a duracao do proprio tone). Um delay sozinho vira
pausa. Cada unidade da tabela vale 5 ms.
"""
import os
import re
import sys

UNIDADE_MS = 5


def main():
    if not 2 <= len(sys.argv) <= 3:
        print(__doc__)
        sys.exit(1)

    arquivos = sys.argv[1:]
    saidas = [converte(a) for a in arquivos]
    destino = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Radar", "Musica6.h"))
    with open(destino, "w", encoding="utf-8") as f:
        f.write("// Gerado por ferramentas/converte_musica_tone.py. Cada unidade = %d ms.\n" % UNIDADE_MS)
        f.write("#define N_EXTRAS %d\n" % len(saidas))
        for n, (nome, linhas) in enumerate(saidas):
            f.write("// Musica %d: %s\n#define TEMPO%d %d\n" % (6 + n, nome, 6 + n, UNIDADE_MS))
            f.write("static const Nota MUS%d[] PROGMEM = {\n" % (6 + n))
            for i in range(0, len(linhas), 8):
                f.write("  " + ",".join(linhas[i:i + 8]) + ("," if i + 8 < len(linhas) else "") + "\n")
            f.write("};\n")
    total = sum(len(l) for _, l in saidas)
    print("%d notas -> %s (cerca de %d bytes de flash)" % (total, destino, total * 3))


def converte(origem):
    texto = open(origem, encoding="utf-8", errors="replace").read()
    texto = re.sub(r"/\*.*?\*/", "", texto, flags=re.S)   # tira comentarios de bloco
    texto = re.sub(r"//[^\n]*", "", texto)                # e de linha

    tokens = re.finditer(
        r"tone\s*\(\s*[^,()]+,\s*(\d+)\s*,\s*(\d+)\s*\)|delay\s*\(\s*(\d+)\s*\)", texto)

    notas = []        # (frequencia, milissegundos)
    pendente = None   # ultimo tone ainda sem delay
    for t in tokens:
        if t.group(1) is not None:                         # tone(pino, hz, dur)
            if pendente:
                notas.append(pendente)
            pendente = (int(t.group(1)), int(t.group(2)))
        else:                                              # delay(ms)
            ms = int(t.group(3))
            if pendente:
                notas.append((pendente[0], ms))
                pendente = None
            else:
                notas.append((0, ms))                      # pausa
    if pendente:
        notas.append(pendente)

    if not notas:
        print("Nao achei nenhum tone()/delay() em", origem)
        sys.exit(1)

    linhas = []
    for hz, ms in notas:
        un = max(1, round(ms / UNIDADE_MS))
        while un > 255:                       # nota mais longa que 1275 ms: continua em pausa
            linhas.append("{%d,%d}" % (hz, 255))
            un -= 255
            hz = 0
        linhas.append("{%d,%d}" % (hz, un))

    return os.path.basename(origem), linhas


if __name__ == "__main__":
    main()
