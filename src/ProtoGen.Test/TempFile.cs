using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Google.ProtocolBuffers.ProtoGen
{
    class ProtoFile : TempFile
    {
        public ProtoFile(string filename, string contents)
            : base(filename, contents)
        {
        }
    }
    class TempFile : IDisposable
    {
        private string tempFile;

        public static TempFile Attach(string path) 
        {
            return new TempFile(path, null);
        }

        protected TempFile(string filename, string contents) {
            tempFile = filename;
            if (contents != null)
            {
                File.WriteAllText(tempFile, contents, new UTF8Encoding(false));
            }
        }

        public TempFile(string contents)
            : this(Path.GetTempFileName(), contents)
        {
        }

        public string TempPath { get { return tempFile; } }

        public void ChangeExtension(string ext)
        {
            string newFile = Path.ChangeExtension(tempFile, ext);
            File.Move(tempFile, newFile);
            tempFile = newFile;
        }

        public void Dispose()
        {
            if (File.Exists(tempFile))
            {
                File.Delete(tempFile);
            }
        }
    }
}
