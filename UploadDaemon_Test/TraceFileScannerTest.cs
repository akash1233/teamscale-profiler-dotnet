﻿using Moq;
using NUnit.Framework;
using System.Collections.Generic;
using System.IO;
using System.IO.Abstractions;
using System.IO.Abstractions.TestingHelpers;
using System.Linq;
using UploadDaemon;

[TestFixture]
public class TraceFileScannerTest
{
    private const string TraceDirectory = @"C:\users\public\traces";

    [Test]
    public void TestAllFileContents()
    {
        string traceContent1 = @"Assembly=VersionAssembly:1 Version:4.0.0.0
Inlined=1:33555646:100678050";
        string traceContent2 = @"Assembly=VersionAssembly:1 Version:4.0.0.0";

        IFileSystem fileSystem = new MockFileSystem(new Dictionary<string, MockFileData>()
        {
            // finished trace
            { FileInTraceDirectory("coverage_1_1.txt"), traceContent1 },
            // empty trace
            { FileInTraceDirectory("coverage_1_2.txt"), traceContent2 },
            // unrelated file
            { FileInTraceDirectory("unrelated.txt"), @"whatever" },
        });

        List<TraceFileScanner.ScannedFile> files = new TraceFileScanner(TraceDirectory, fileSystem).ListTraceFilesReadyForUpload().ToList();

        Assert.That(files, Is.EquivalentTo(new TraceFileScanner.ScannedFile[] {
            new TraceFileScanner.ScannedFile()
            {
                FilePath = FileInTraceDirectory("coverage_1_1.txt"),
                Lines = traceContent1.SplitLines(),
                IsEmpty = false,
            },
            new TraceFileScanner.ScannedFile()
            {
                FilePath = FileInTraceDirectory("coverage_1_2.txt"),
                Lines = traceContent2.SplitLines(),
                IsEmpty = true,
            },
        }));
    }

    [Test]
    public void LockedFileShouldBeIgnored()
    {
        IFileSystem fileSystemMock = FileSystemMockingUtils.MockFileSystem(fileMock =>
        {
            fileMock.Setup(file => file.Open("coverage_1_1.txt", It.IsAny<FileMode>())).Throws<IOException>();
        }, directoryMock =>
        {
            directoryMock.Setup(directory => directory.EnumerateFiles(It.IsAny<string>()))
                .Returns(new string[] { "coverage_1_1.txt" });
        }).Object;

        List<TraceFileScanner.ScannedFile> files =
            new TraceFileScanner(TraceDirectory, fileSystemMock).ListTraceFilesReadyForUpload().ToList();

        Assert.That(files, Is.Empty);
    }

    [Test]
    public void ExceptionsShouldLeadToFileBeingIgnored()
    {
        string[] traceContent = new string[] { "Inlined=1:33555646:100678050" };

        IFileSystem fileSystemMock = FileSystemMockingUtils.MockFileSystem(fileMock =>
        {
            fileMock.Setup(file => file.ReadAllLines("coverage_1_1.txt")).Throws<IOException>();
            fileMock.Setup(file => file.ReadAllLines("coverage_1_2.txt")).Returns(traceContent);
        }, directoryMock =>
        {
            directoryMock.Setup(directory => directory.EnumerateFiles(It.IsAny<string>()))
                .Returns(new string[] { "coverage_1_1.txt", "coverage_1_2.txt" });
        }).Object;

        List<TraceFileScanner.ScannedFile> files =
            new TraceFileScanner(TraceDirectory, fileSystemMock).ListTraceFilesReadyForUpload().ToList();

        Assert.That(files, Is.EquivalentTo(new TraceFileScanner.ScannedFile[] {
            new TraceFileScanner.ScannedFile()
            {
                FilePath = "coverage_1_2.txt",
                Lines = traceContent,
                IsEmpty = false,
            },
        }));
    }

    /// <summary>
    /// Returns a file with the given name in the trace directory.
    /// </summary>
    private string FileInTraceDirectory(string fileName)
    {
        return Path.Combine(TraceDirectory, fileName);
    }
}