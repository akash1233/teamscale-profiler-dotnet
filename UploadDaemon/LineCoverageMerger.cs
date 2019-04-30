﻿using Common;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UploadDaemon.SymbolAnalysis;
using UploadDaemon.Upload;
using static UploadDaemon.SymbolAnalysis.LineCoverageSynthesizer;

namespace UploadDaemon
{
    /// <summary>
    /// Merges all line coverage that will be uploaded for the same timestamp/revision into one "batch".
    /// This allows uploading the coverage in one go instead of splitting it into multiple requests and thus
    /// commits in Teamscale.
    /// </summary>
    internal class LineCoverageMerger
    {
        private class MergeKey
        {
            public RevisionFileUtils.RevisionOrTimestamp RevisionOrTimestamp;
            public IUpload Upload;
        }

        /// <summary>
        /// Groups all line coverage that should be uploaded in one batch.
        /// </summary>
        public class CoverageBatch
        {
            /// <summary>
            /// The revision or timestamp to which the coverage should be uploaded.
            /// </summary>
            public RevisionFileUtils.RevisionOrTimestamp RevisionOrTimestamp { get; }

            /// <summary>
            /// The upload to use for this batch.
            /// </summary>
            public IUpload Upload { get; }

            /// <summary>
            /// The line coverage that should be uploaded.
            /// </summary>
            public Dictionary<string, FileCoverage> LineCoverage { get; } = new Dictionary<string, FileCoverage>();

            /// <summary>
            /// The original trace files from which the line coverage was generated. Used for logging.
            /// </summary>
            public List<string> TraceFilePaths { get; }

            public CoverageBatch(IUpload upload, RevisionFileUtils.RevisionOrTimestamp revisionOrTimestamp)
            {
                this.Upload = upload;
                this.RevisionOrTimestamp = revisionOrTimestamp;
            }
        }

        private readonly Dictionary<MergeKey, CoverageBatch> mergedCoverage = new Dictionary<MergeKey, CoverageBatch>();

        /// <summary>
        /// Adds the given line coverage from the given file, that should be uploaded to the
        /// given revision/timestamp to the given upload to the merger. The coverage will be merged with
        /// any existing coverage that should be uploaded to the same destination.
        /// </summary>
        public void AddLineCoverage(string traceFilePath, RevisionFileUtils.RevisionOrTimestamp revisionOrTimestamp, IUpload upload, Dictionary<string, FileCoverage> lineCoverage)
        {
            MergeKey key = new MergeKey
            {
                RevisionOrTimestamp = revisionOrTimestamp,
                Upload = upload,
            };

            if (!mergedCoverage.TryGetValue(key, out CoverageBatch batch))
            {
                batch = new CoverageBatch(key.Upload, key.RevisionOrTimestamp);
            }

            batch.TraceFilePaths.Add(traceFilePath);

            foreach (string file in lineCoverage.Keys)
            {
                if (!batch.LineCoverage.ContainsKey(file))
                {
                    batch.LineCoverage[file] = new FileCoverage();
                }
                batch.LineCoverage[file].CoveredLineRanges.UnionWith(lineCoverage[file].CoveredLineRanges);
            }
        }

        /// <summary>
        /// Returns all batches the merger has created.
        /// </summary>
        public IEnumerable<CoverageBatch> GetBatches()
        {
            return mergedCoverage.Values;
        }
    }
}
