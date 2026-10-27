using System.Text;

namespace LazyCamping.PluginBuilder;

/// <summary>
/// Rebuilds a plugin Mutagen wrote, taking every record that wasn't changed
/// byte for byte from the original file instead. Mutagen's own writing is
/// faithful but not identical (it reorders package data inputs and
/// recompresses records), and the Creation Kit is picky, so records the
/// update didn't touch stay exactly as the Creation Kit saved them.
/// </summary>
public static class Splice
{
    public static byte[] Run(byte[] original, byte[] written, IReadOnlySet<uint> changed)
    {
        var originals = new Dictionary<uint, (int Start, int Length)>();
        Index(original, HeaderEnd(original), original.Length, originals);

        using var output = new MemoryStream();
        var headerEnd = HeaderEnd(written);
        output.Write(written, 0, headerEnd);
        Copy(written, headerEnd, written.Length, output, original, originals, changed);
        return output.ToArray();
    }

    static int HeaderEnd(byte[] data) => 24 + BitConverter.ToInt32(data, 4);

    static void Index(byte[] data, int start, int end, Dictionary<uint, (int, int)> records)
    {
        for (var offset = start; offset < end;)
        {
            var size = BitConverter.ToInt32(data, offset + 4);
            if (Encoding.ASCII.GetString(data, offset, 4) == "GRUP")
            {
                Index(data, offset + 24, offset + size, records);
                offset += size;
            }
            else
            {
                records[BitConverter.ToUInt32(data, offset + 12)] = (offset, 24 + size);
                offset += 24 + size;
            }
        }
    }

    static void Copy(byte[] data, int start, int end, MemoryStream output, byte[] original,
        Dictionary<uint, (int Start, int Length)> originals, IReadOnlySet<uint> changed)
    {
        for (var offset = start; offset < end;)
        {
            var size = BitConverter.ToInt32(data, offset + 4);
            if (Encoding.ASCII.GetString(data, offset, 4) == "GRUP")
            {
                var groupStart = output.Position;
                output.Write(data, offset, 24);
                Copy(data, offset + 24, offset + size, output, original, originals, changed);
                // The group's size covers its header and contents.
                var groupEnd = output.Position;
                output.Position = groupStart + 4;
                output.Write(BitConverter.GetBytes((int)(groupEnd - groupStart)));
                output.Position = groupEnd;
                offset += size;
            }
            else
            {
                var formId = BitConverter.ToUInt32(data, offset + 12);
                if (!changed.Contains(formId) && originals.TryGetValue(formId, out var kept))
                {
                    output.Write(original, kept.Start, kept.Length);
                }
                else
                {
                    output.Write(data, offset, 24 + size);
                }
                offset += 24 + size;
            }
        }
    }
}
