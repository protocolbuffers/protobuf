using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Google.ProtocolBuffers.ProtoGen
{
	class ProtoFile : TempFile
	{
		public ProtoFile (string filename, string contents)
		{
			_tempFile = filename;
			File.WriteAllText(_tempFile, contents);
		}
	}
	class TempFile : IDisposable
	{
		protected string _tempFile;

		public static TempFile Attach(string path) 
		{
			TempFile f = new TempFile();
			f._tempFile = path;
			return f;
		}

		protected TempFile() { }
		public TempFile(string contents)
		{
			File.WriteAllText(_tempFile = Path.GetTempFileName(), contents, Encoding.ASCII);
		}

		public string TempPath { get { return _tempFile; } }

		public void ChangeExtension(string ext)
		{
			string newFile = Path.ChangeExtension(_tempFile, ext);
			File.Move(_tempFile, newFile);
			_tempFile = newFile;
		}

		public void Dispose()
		{
			if(File.Exists(_tempFile))
				File.Delete(_tempFile);
		}
	}
}
