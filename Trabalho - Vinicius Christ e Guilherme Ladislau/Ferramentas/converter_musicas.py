import os
import re
import sys

MAX_MUSICAS = 4


def sem_comentarios(texto):
    texto = re.sub(r"/\*.*?\*/", "", texto, flags=re.S)
    return re.sub(r"//[^\n]*", "", texto)


def para_unidades(notas, unidade_ms):
    linhas = []
    for hz, ms in notas:
        un = max(1, round(ms / unidade_ms))
        while un > 255:
            linhas.append("{%d,%d}" % (hz, 255))
            un -= 255
            hz = 0
        linhas.append("{%d,%d}" % (hz, un))
    return linhas


def ler_tone(texto):
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


def ler_melodias(texto):
    valores = {"R": 0}
    for nome, v in re.findall(r"#define\s+(\w+)\s+(\d+)", texto):
        valores[nome] = int(v)
    m = re.search(r"tempo\s*=\s*(\d+)", texto)
    tempo = int(m.group(1)) if m else 10000

    def vetor(nome):
        m = re.search(r"\b%s\s*\[\s*\]\s*=\s*\{([^}]*)\}" % nome, texto)
        return [x.strip() for x in m.group(1).split(",") if x.strip()] if m else None

    musicas = []
    for n in dict.fromkeys(re.findall(r"\bmelody(\d+)\s*\[", texto)):
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


def converter(origem):
    texto = sem_comentarios(open(origem, encoding="utf-8", errors="replace").read())
    base = os.path.basename(origem)
    saidas = []
    melodias = ler_melodias(texto)
    if melodias:
        for nome, notas in melodias:
            saidas.append(("%s (%s)" % (base, nome), para_unidades(notas, 10), 10))
    else:
        notas = ler_tone(texto)
        if notas:
            saidas.append((base, para_unidades(notas, 5), 5))
    if not saidas:
        print("Nao achei musica em", origem)
        sys.exit(1)
    return saidas


def pastas_dos_codigos():
    raiz = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
    candidatas = [
        os.path.join(raiz, "Arduino", "RadarPanTilt"),
        os.path.join(raiz, "SimulIDE", "RadarPanTilt_SimulIDE"),
    ]
    pastas = [os.path.normpath(p) for p in candidatas if os.path.isdir(p)]
    if not pastas:
        print("Nao achei as pastas Arduino e SimulIDE ao lado de Ferramentas.")
        sys.exit(1)
    return pastas


def main():
    if len(sys.argv) < 2:
        print("Uso: python converter_musicas.py musica1.ino [musica2.ino ...]")
        sys.exit(1)

    musicas = []
    for arquivo in sys.argv[1:]:
        musicas += converter(arquivo)
    if len(musicas) > MAX_MUSICAS:
        print("Sao %d musicas e o maximo e %d." % (len(musicas), MAX_MUSICAS))
        sys.exit(1)

    texto = "#define N_EXTRAS %d\n" % len(musicas)
    for n, (nome, linhas, unidade) in enumerate(musicas):
        texto += "#define TEMPO%d %d\n" % (6 + n, unidade)
        texto += "static const Nota MUS%d[] PROGMEM = {\n" % (6 + n)
        for i in range(0, len(linhas), 8):
            texto += "  " + ",".join(linhas[i:i + 8]) + ("," if i + 8 < len(linhas) else "") + "\n"
        texto += "};\n"

    for pasta in pastas_dos_codigos():
        destino = os.path.join(pasta, "MusicasExtras.h")
        with open(destino, "w", encoding="utf-8", newline="\n") as f:
            f.write(texto)
        print("gravado:", destino)

    for n, (nome, linhas, _) in enumerate(musicas):
        print("tecla %d: %s (%d notas)" % (6 + n, nome, len(linhas)))


if __name__ == "__main__":
    main()
