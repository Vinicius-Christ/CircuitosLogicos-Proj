"""
Converte sketches de musica do Arduino para as tabelas de notas do RadarPanTilt (teclas 6 a 9).

Uso (no Prompt de Comando, dentro da pasta do projeto):
    python ferramentas\converter_musicas.py arquivo1.ino [arquivo2.ino ...]

Gera MusicasExtras.h na pasta do sketch (RadarPanTilt). Com esse arquivo, o RadarPanTilt ganha as musicas
6, 7, 8 e 9 (na ordem em que forem convertidas, no maximo 4). Sem ele, so existem as 5
musicas de sempre. Compile e grave de novo no Arduino depois de gerar.

Formatos aceitos:
  1. tone(pino, frequencia, duracao) seguido de delay(ms): cada tone vira uma nota, e o
     delay logo depois define o tempo total dela. Um delay sozinho vira pausa.
  2. Vetores melodyN[] (periodo da nota em microssegundos, com #define das notas) e
     beatsN[] (duracao relativa), com "long tempo = N;": cada melodyN vira uma musica.
     Uma duracao de beat vale beat * tempo / 1000 ms.
"""
import os
import re
import sys

MAX_MUSICAS = 4


def sem_comentarios(texto):
    texto = re.sub(r"/\*.*?\*/", "", texto, flags=re.S)
    return re.sub(r"//[^\n]*", "", texto)


def para_unidades(notas, unidade_ms):
    """notas = [(hz, ms)] -> ['{hz,un}'], quebrando notas mais longas que 255 unidades."""
    linhas = []
    for hz, ms in notas:
        un = max(1, round(ms / unidade_ms))
        while un > 255:
            linhas.append("{%d,%d}" % (hz, 255))
            un -= 255
            hz = 0
        linhas.append("{%d,%d}" % (hz, un))
    return linhas


def converte_tone(texto):
    tokens = re.finditer(
        r"tone\s*\(\s*[^,()]+,\s*(\d+)\s*,\s*(\d+)\s*\)|delay\s*\(\s*(\d+)\s*\)", texto)
    notas = []
    pendente = None
    for t in tokens:
        if t.group(1) is not None:
            if pendente:
                notas.append(pendente)
            pendente = (int(t.group(1)), int(t.group(2)))
        else:
            ms = int(t.group(3))
            if pendente:
                notas.append((pendente[0], ms))
                pendente = None
            else:
                notas.append((0, ms))
    if pendente:
        notas.append(pendente)
    return notas


def converte_melodias(texto):
    """Formato melodyN[] / beatsN[]. Devolve [(nome, [(hz, ms)])]."""
    valores = {"R": 0}
    for nome, v in re.findall(r"#define\s+(\w+)\s+(\d+)", texto):
        valores[nome] = int(v)
    m = re.search(r"tempo\s*=\s*(\d+)", texto)
    tempo = int(m.group(1)) if m else 10000

    def vetor(nome):
        m = re.search(r"\b%s\s*\[\s*\]\s*=\s*\{([^}]*)\}" % nome, texto)
        return [x.strip() for x in m.group(1).split(",") if x.strip()] if m else None

    musicas = []
    for n in dict.fromkeys(re.findall(r"\bmelody(\d+)\s*\[", texto)):   # sem repetir
        mel, bat = vetor("melody" + n), vetor("beats" + n)
        if not mel or not bat:
            continue
        notas = []
        for periodo, beat in zip(mel, bat):
            p = valores[periodo] if periodo in valores else int(periodo)
            hz = round(1000000 / p) if p > 0 else 0
            notas.append((hz, int(beat) * tempo / 1000))
        musicas.append(("melody" + n, notas))
    return musicas


def converte(origem):
    """Devolve [(nome, linhas, unidade_ms)] para cada musica do arquivo."""
    texto = sem_comentarios(open(origem, encoding="utf-8", errors="replace").read())
    base = os.path.basename(origem)
    saidas = []
    melodias = converte_melodias(texto)
    if melodias:
        for nome, notas in melodias:
            saidas.append(("%s (%s)" % (base, nome), para_unidades(notas, 10), 10))
    else:
        notas = converte_tone(texto)
        if notas:
            saidas.append((base, para_unidades(notas, 5), 5))
    if not saidas:
        print("Nao achei musica em", origem)
        sys.exit(1)
    return saidas


def pasta_do_sketch():
    raiz = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
    for nome in ("RadarPanTilt",):
        pasta = os.path.normpath(os.path.join(raiz, nome))
        if os.path.isdir(pasta):
            return pasta
    print("Nao achei a pasta RadarPanTilt ao lado de ferramentas.")
    sys.exit(1)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    musicas = []
    for arquivo in sys.argv[1:]:
        musicas += converte(arquivo)
    if len(musicas) > MAX_MUSICAS:
        print("Sao %d musicas e o maximo e %d." % (len(musicas), MAX_MUSICAS))
        sys.exit(1)

    destino = os.path.join(pasta_do_sketch(), "MusicasExtras.h")
    with open(destino, "w", encoding="utf-8") as f:
        f.write("// Gerado por ferramentas/converter_musicas.py\n")
        f.write("#define N_EXTRAS %d\n" % len(musicas))
        for n, (nome, linhas, unidade) in enumerate(musicas):
            f.write("// Musica %d: %s\n#define TEMPO%d %d\n" % (6 + n, nome, 6 + n, unidade))
            f.write("static const Nota MUS%d[] PROGMEM = {\n" % (6 + n))
            for i in range(0, len(linhas), 8):
                f.write("  " + ",".join(linhas[i:i + 8]) + ("," if i + 8 < len(linhas) else "") + "\n")
            f.write("};\n")

    for n, (nome, linhas, _) in enumerate(musicas):
        print("tecla %d: %s (%d notas)" % (6 + n, nome, len(linhas)))
    total = sum(len(l) for _, l, _ in musicas)
    print("-> %s (cerca de %d bytes de flash)" % (destino, total * 3))


if __name__ == "__main__":
    main()
