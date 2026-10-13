param(
    [string]$Archive = (Join-Path $PWD 'ps3data.psarc'),
    [string]$OutputDirectory = (Join-Path $PWD 'ps3-ui-assets'),
    [switch]$ListOnly
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$typeOptions = @{}
if ($PSVersionTable.PSEdition -eq 'Desktop') {
    $typeOptions.ReferencedAssemblies = @('System.dll', 'System.IO.Compression.dll')
}
Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.IO.Compression;
using System.Collections.Generic;
using System.Text;

public static class MotoGPPSARC {
    static byte[] Read(Stream stream, int count) {
        var bytes = new byte[count];
        int done = 0;
        while (done < count) {
            int n = stream.Read(bytes, done, count - done);
            if (n == 0) throw new EndOfStreamException("Archive is truncated.");
            done += n;
        }
        return bytes;
    }
    static long BE(byte[] bytes, int offset, int count) {
        long value = 0;
        for (int i = 0; i < count; ++i) value = (value << 8) | bytes[offset + i];
        return value;
    }
    static byte[] Inflate(byte[] bytes) {
        if (bytes.Length < 6 || (bytes[0] & 15) != 8 ||
            ((bytes[0] << 8) + bytes[1]) % 31 != 0 || (bytes[1] & 32) != 0)
            throw new InvalidDataException("Unsupported zlib block header.");
        // PS 5.1 / .NET Framework has DeflateStream but no ZLibStream.
        // Remove the zlib wrapper and verify its Adler-32 after decompression.
        using (var source = new MemoryStream(bytes, 2, bytes.Length - 6, false))
        using (var deflate = new DeflateStream(source, CompressionMode.Decompress))
        using (var result = new MemoryStream()) {
            var buffer = new byte[8192];
            int count;
            while ((count = deflate.Read(buffer, 0, buffer.Length)) != 0) {
                if (result.Length + count > 1048576) throw new InvalidDataException("Block exceeds limit.");
                result.Write(buffer, 0, count);
            }
            var data = result.ToArray();
            uint a = 1, b = 0;
            foreach (byte value in data) { a = (a + value) % 65521; b = (b + a) % 65521; }
            if (((b << 16) | a) != (uint)BE(bytes, bytes.Length - 4, 4))
                throw new InvalidDataException("Archive block checksum failed.");
            return data;
        }
    }
    class ArchiveReader : IDisposable {
        public FileStream File;
        byte[] toc;
        int entrySize, entries, blockSize, blockWidth, lengthsOffset;
        public ArchiveReader(string path) {
            File = System.IO.File.OpenRead(path);
            try {
                byte[] header = Read(File, 32);
                if (Encoding.ASCII.GetString(header, 0, 4) != "PSAR" ||
                    BE(header, 4, 4) != 0x10004 ||
                    Encoding.ASCII.GetString(header, 8, 4) != "zlib" || BE(header, 28, 4) != 0)
                    throw new InvalidDataException("Expected an unencrypted PSARC 1.4 zlib archive.");
                int tocLength = checked((int)BE(header, 12, 4));
                entrySize = checked((int)BE(header, 16, 4));
                entries = checked((int)BE(header, 20, 4));
                blockSize = checked((int)BE(header, 24, 4));
                if (tocLength < 32 || tocLength > 16777216 || entrySize != 30 || entries < 1 ||
                    blockSize < 256 || blockSize > 1048576 || (blockSize & (blockSize - 1)) != 0)
                    throw new InvalidDataException("Invalid archive table dimensions.");
                blockWidth = 1;
                for (int value = blockSize - 1; value > 255; value >>= 8) ++blockWidth;
                toc = Read(File, tocLength - 32);
                lengthsOffset = checked(entrySize * entries);
                if (lengthsOffset > toc.Length) throw new InvalidDataException("Invalid archive entry count.");
            } catch { File.Dispose(); throw; }
        }
        public void Extract(int entry, Stream output, long limit) {
            if (entry < 0 || entry >= entries) throw new InvalidDataException("Invalid entry index.");
            int start = entry * entrySize;
            int block = checked((int)BE(toc, start + 16, 4));
            long size = BE(toc, start + 20, 5), offset = BE(toc, start + 25, 5);
            if (size > limit || offset < toc.Length + 32 || offset > File.Length)
                throw new InvalidDataException("Entry size or offset is invalid.");
            File.Position = offset;
            long remaining = size;
            while (remaining != 0) {
                int lengthPosition = checked(lengthsOffset + block++ * blockWidth);
                if (lengthPosition + blockWidth > toc.Length) throw new InvalidDataException("Missing block length.");
                int packed = (int)BE(toc, lengthPosition, blockWidth);
                if (packed == 0) packed = blockSize;
                if (packed > blockSize) throw new InvalidDataException("Oversized archive block.");
                byte[] data = Read(File, packed);
                if (packed != blockSize) data = Inflate(data);
                int expected = (int)Math.Min(remaining, blockSize);
                if (data.Length < expected || data.Length > blockSize)
                    throw new InvalidDataException("Unexpected decompressed block length.");
                output.Write(data, 0, expected);
                remaining -= expected;
            }
        }
        public string[] Names() {
            using (var names = new MemoryStream()) {
                Extract(0, names, 8388608);
                string text = new UTF8Encoding(false, true).GetString(names.ToArray());
                string[] lines = text.TrimEnd('\n', '\r', '\0').Split('\n');
                if (lines.Length != entries - 1) throw new InvalidDataException("Manifest entry count mismatch.");
                for (int i = 0; i < lines.Length; ++i) lines[i] = lines[i].TrimEnd('\r');
                return lines;
            }
        }
        public void Dispose() { File.Dispose(); }
    }
    public static string[] List(string path) {
        using (var archive = new ArchiveReader(path)) return archive.Names();
    }
    public static void ExtractUI(string path, string outputDirectory) {
        using (var archive = new ArchiveReader(path)) {
            var names = archive.Names();
            var indices = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
            for (int i = 0; i < names.Length; ++i) indices.Add(names[i], i + 1);
            string[] wanted = { "frontend.mgb", "sharedfrontend.mgb", "ingame.mgb" };
            foreach (string name in wanted) if (!indices.ContainsKey(name))
                throw new FileNotFoundException("Archive does not contain " + name);
            var selected = new List<string>(wanted);
            foreach (string font in new [] { "arial_bold_17pt", "arial_bold_small", "impact_17pt", "impact_small" })
                foreach (string extension in new [] { ".font", ".zones" }) {
                    string name = "fonts/" + font + extension;
                    if (indices.ContainsKey(name)) selected.Add(name);
                }
            Directory.CreateDirectory(outputDirectory);
            foreach (string name in selected) {
                string destination = Path.Combine(outputDirectory, name), partial = destination + ".partial";
                if (System.IO.File.Exists(destination) || System.IO.File.Exists(partial))
                    throw new IOException("Output already exists: " + destination + ". Choose a new OutputDirectory.");
                try {
                    Directory.CreateDirectory(Path.GetDirectoryName(destination));
                    using (var file = new FileStream(partial, FileMode.CreateNew, FileAccess.Write))
                        archive.Extract(indices[name], file, 268435456);
                    System.IO.File.Move(partial, destination);
                } catch { if (System.IO.File.Exists(partial)) System.IO.File.Delete(partial); throw; }
                Console.WriteLine("Extracted " + name);
            }
        }
    }
}
'@ @typeOptions

$Archive = (Resolve-Path -LiteralPath $Archive).Path
if ($ListOnly) {
    [MotoGPPSARC]::List($Archive)
    return
}
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$zip = $OutputDirectory + '.zip'
if (Test-Path -LiteralPath $zip) { throw "Output already exists: $zip" }
[MotoGPPSARC]::ExtractUI($Archive, $OutputDirectory)
[IO.Compression.ZipFile]::CreateFromDirectory($OutputDirectory, $zip)
Write-Host "Upload this file: $zip"
