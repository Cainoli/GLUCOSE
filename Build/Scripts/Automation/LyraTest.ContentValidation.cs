// Copyright Epic Games, Inc.All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using AutomationTool;
using EpicGame;
using EpicGames.Core;
using Microsoft.Extensions.Logging;

using static AutomationTool.CommandUtils;

namespace LyraTest
{
	[Help("Commandlet for checking content to know if passes all the existing UEditorValidators.")]
	[Help("Branch=<Name>", "Branch to use")]
	[Help("CL=<value>", "Check file changes in the range LastCL,CL")]
	[Help("p4shelved", "If specified, treat CL as a shelved file to check the contents")]
	[Help("p4opened", "Check currently opened files instead of using CL ranged")]
	[Help("MaxPackagesToLoad=<value>", "Maximum number of recent changes to check")]
	[Help("LastGoodContentCLPath=<value>", "A directory location to store the 'last good' CL so we can determine CLs between runs.")]
	[Help("SkipPrevCLFileExport", "Skip exporting the 'last good' CL.")]
	public class LyraContentValidation : BuildCommand
	{
		public override void ExecuteBuild()
		{
			int ThisCLInt = 0;
			int PrevCL = 0;
			bool CheckOpenedFiles = ParseParam("opened");
			bool CheckShelvedFiles = ParseParam("shelved");
			string CommandletArgs = "";
			bool RunCommandlet = false;
			string PrevCLFilePath = "";
			bool SkipPrevCLFileExport = ParseParam("SkipPrevCLFileExport");

			//@TODO: Should derive these, otherwise it will break as soon as someone clones the projects
			string GameProjectDirectory = "Samples/Games/Lyra";
			string GameProject = "Lyra.uproject";

			string ExtensionTypeListParam = ParseParamValue("ExtTypeList", ".uasset,.umap,.cpp,.c,.h,.inl,.ini,.uproject,.uplugin,.json");
			List<string> ExtensionTypeList = new List<string>();
			if (string.IsNullOrEmpty(ExtensionTypeListParam))
			{
				Logger.LogInformation("No extensions were passed in, defaulting to always run.  Set -ExtTypeList to the extension typelist for triggering the commandlet");
				RunCommandlet = true;
			}
			else
			{
				ExtensionTypeList = ExtensionTypeListParam.Split(',').ToList();
			}

			// if not checking open files, use CL ranges
			if (CheckOpenedFiles)
			{
				// Quickly check if the CL has any files we're interested in. This is significantly faster than firing up the editor to do
				// nothing
				RunCommandlet = AreFileTypesModifiedInOpenChangelists(ExtensionTypeList);

				// save anyone who forgot to run this without -p4
				if (!P4Enabled)
				{
					throw new AutomationException("This script must be executed with -p4");
				}
				CommandletArgs = "-P4Opened -P4Client=" + P4Env.Client;
			}
			else
			{
				string ThisCL = ParseParamValue("CL");
				if (string.IsNullOrEmpty(ThisCL))
				{
					throw new AutomationException("-CL=<num> or -opened must be specified.");
				}

				if (!int.TryParse(ThisCL, out ThisCLInt))
				{
					throw new AutomationException("-CL must be a number.");
				}

				if (CheckShelvedFiles)
				{
					// Quickly check if the CL has any files we're interested in. This is significantly faster than firing up the editor to do
					// nothing
					RunCommandlet = AreFileTypesModifiedInShelvedChangelist(ThisCL, ExtensionTypeList);
					// filter is what's passed to p4 files. If shelved we'll use the syntax that pulls the shelved file list
					CommandletArgs = String.Format("-P4Filter=@={0}", ThisCL);
				}
				else
				{
					string Branch = ParseParamValue("Branch");
					if (string.IsNullOrEmpty(Branch))
					{
						throw new AutomationException("-Branch must be specified to check a CL range when -opened or -shelved are not present");
					}

					string LastGoodContentCLPath = ParseParamValue("LastGoodContentCLPath");
					if (string.IsNullOrEmpty(LastGoodContentCLPath))
					{
						// Default to local storage for this file (Legacy behavior)
						LastGoodContentCLPath = CombinePaths(CmdEnv.LocalRoot, "Engine", "Saved");
					}

					string PrevCLFileName = "PrevCL_" + Branch.Replace("/", "+") + ".txt";
					PrevCLFilePath = CombinePaths(LastGoodContentCLPath, PrevCLFileName);
					PrevCL = ReadPrevCLFile(PrevCLFilePath);

					if (PrevCL <= 0)
					{
						Logger.LogInformation("Previous CL file didn't exist. Defaulting to none!");
						RunCommandlet = true;
					}
					else if (PrevCL >= ThisCLInt)
					{
						Logger.LogInformation("Previous CL file shows a CL equal or newer than the current CL. This content was already checked. Skipping.");
						RunCommandlet = false;
					}
					else
					{
						// +1 to the previous cl so it won't use content from the previous change
						PrevCL++;
						CommandletArgs = String.Format("-P4Filter={0}/{1}/...@{2},{3}", Branch, GameProjectDirectory, PrevCL, ThisCL);
						Logger.LogInformation("Generated Filter: {CommandletArgs}", CommandletArgs);

						RunCommandlet = WereFileTypesModifiedInChangelistRange(Branch, PrevCL, ThisCL, ExtensionTypeList);
					}

					if (!RunCommandlet)
					{
						Logger.LogInformation("No files in CL Range {PrevCL} -> {ThisCL} contained any files ending with extensions {ExtensionTypeListParam}, or they were already checked in a previous job, skipping commandlet run", PrevCL, ThisCL, ExtensionTypeListParam);
					}
				}
			}

			if (RunCommandlet)
			{
				string EditorExe = "UnrealEditor.exe";
				EditorExe = AutomationTool.HostPlatform.Current.GetUnrealExePath(EditorExe);

				string PerforceToken = P4.GetAuthenticationToken();

				CommandletArgs += " -ini:Engine:[Core.System]:AssetLogShowsDiskPath=True  -LogCmds=\"LogHttp Error\" ";

				// Do we have a valid perforce token?
				//if (!string.IsNullOrEmpty(PerforceToken))
				{
					CommandletArgs += String.Format(" -SCCProvider={0} -P4Port={1} -P4User={2} -P4Client={3} -P4Passwd={4}", "Perforce", P4Env.ServerAndPort, P4Env.User, P4Env.Client, PerforceToken);

					// If skip export was specified (e.g. a preflight) don't export anything
				}

				string MaxPackagesToLoad = ParseParamValue("MaxPackagesToLoad", "2000");
				CommandletArgs += String.Format(" -MaxPackagesToLoad={0}", MaxPackagesToLoad);
				CommandUtils.RunCommandlet(new FileReference(CombinePaths(CmdEnv.LocalRoot, GameProjectDirectory, GameProject)), EditorExe, "ContentValidationCommandlet", CommandletArgs);
			}

			// Read the previous CL file one more time before writing to it, in case it changed while we were running
			if (SkipPrevCLFileExport)
			{
				Logger.LogInformation("Not writing PrevCLFile as -SkipPrevCLFileExport was specified");
			}
			else
			{
				ExportPrevCL(PrevCL, ThisCLInt, PrevCLFilePath);
			}
		}

		/// <summary>
		/// Exports 'last good' CL information by reading the previous CL file before writing the current CL to it, in case it changed while we were running
		/// </summary>
		/// <param name="PrevCL"></param>
		/// <param name="ThisCL"></param>
		/// <param name="PrevCLFilePath"></param>
		private void ExportPrevCL(int PrevCL, int ThisCL, string PrevCLFilePath)
		{
			if (ThisCL > 0)
			{
				if (PrevCL < ThisCL)
				{
					PrevCL = ReadPrevCLFile(PrevCLFilePath);
				}

				if (PrevCL < ThisCL)
				{
					Logger.LogInformation("Writing PrevCLFile {PrevCLFilePath}...", PrevCLFilePath);
					WritePrevCLFile(PrevCLFilePath, ThisCL.ToString());
				}
				else
				{
					Logger.LogInformation("Not writing PrevCLFile {PrevCLFilePath}. The current CL was not newer", PrevCLFilePath);
				}
			}
			else
			{
				Logger.LogInformation("Not writing PrevCLFile {PrevCLFilePath} as the current CL {ThisCL} was invalid", PrevCLFilePath, ThisCL);
			}
		}

		private int ReadPrevCLFile(string PrevCLFilePath)
		{
			int RetVal = 0;
			if (File.Exists(PrevCLFilePath))
			{
				string PrevCLString = "";
				int RetryCount = 10;
				bool bProceed = false;
				do
				{
					try
					{
						PrevCLString = File.ReadAllText(PrevCLFilePath);
						bProceed = true;
					}
					catch (Exception Ex)
					{
						if (RetryCount > 0)
						{
							Logger.LogInformation("Failed to read PrevCLFilePath {PrevCLFilePath}. Retrying in a few seconds. Ex:{Arg1}", PrevCLFilePath, Ex.Message);
							RetryCount--;
							Thread.Sleep(TimeSpan.FromSeconds(5));
						}
						else
						{
							Logger.LogError("Failed to read PrevCLFilePath {PrevCLFilePath}. All Retries exhausted, skipping. Ex:{Arg1}", PrevCLFilePath, Ex.Message);
							bProceed = true;
						}
					}
				} while (!bProceed);

				if (int.TryParse(PrevCLString, out RetVal))
				{
					// Read the file successfully, and it was a number
				}
				else
				{
					Logger.LogWarning("{Text}", "Couldn't parse out the changelist number from the saved PrevCLFilePath file. " + PrevCLFilePath);
				}
			}

			return RetVal;
		}

		private void WritePrevCLFile(string PrevCLFilePath, string ThisCL)
		{
			int RetryCount = 10;
			bool bProceed = false;
			do
			{
				try
				{
					Directory.CreateDirectory(Path.GetDirectoryName(PrevCLFilePath));
					File.WriteAllText(PrevCLFilePath, ThisCL);
					bProceed = true;
				}
				catch (Exception Ex)
				{
					if (RetryCount > 0)
					{
						Logger.LogInformation("Failed to write PrevCLFilePath {PrevCLFilePath}. Retrying in a few seconds. Ex:{Arg1}", PrevCLFilePath, Ex.Message);
						RetryCount--;
						Thread.Sleep(TimeSpan.FromSeconds(5));
					}
					else
					{
						Logger.LogError("Failed to write PrevCLFilePath {PrevCLFilePath}. All Retries exhausted, skipping. Ex:{Arg1}", PrevCLFilePath, Ex.Message);
						bProceed = true;
					}
				}
			} while (!bProceed);
		}

		/// <summary>
		/// Returns true if files with extensions in the provided list were modified in the specified changelist range
		/// </summary>
		/// <param name="Branch"></param>
		/// <param name="PrevCL"></param>
		/// <param name="ThisCL"></param>
		/// <param name="ExtensionTypeList"></param>
		/// <returns></returns>
		private bool WereFileTypesModifiedInChangelistRange(string Branch, int PrevCL, string ThisCL, List<string> ExtensionTypeList)
		{
			// we don't need to do any of this if there was no extensions typelist passed in
			if (ExtensionTypeList.Count != 0)
			{
				// check all the changes in FN from PrevCL to now
				List<P4Connection.ChangeRecord> ChangeRecords;
				CommandUtils.P4.Changes(out ChangeRecords, string.Format("{0}/...@{1},{2}", Branch, PrevCL, ThisCL), false);
				foreach (P4Connection.ChangeRecord Record in ChangeRecords)
				{
					P4Connection.DescribeRecord DescribeRecord;
					CommandUtils.P4.DescribeChangelist(Record.CL, out DescribeRecord, false);
					// check all the files in each cl record
					foreach (P4Connection.DescribeRecord.DescribeFile File in DescribeRecord.Files)
					{
						// if any of them end in extensions in our typelist, we need to build
						foreach (string Extension in ExtensionTypeList)
						{
							if (File.File.EndsWith(Extension))
							{
								return true;
							}
						}
					}
				}
			}

			return false;
		}

		/// <summary>
		/// Returns true if files with extensions in the provided list are in the specified shelved changelist
		/// </summary>
		/// <param name="ShelvedChangelist"></param>
		/// <param name="ExtensionTypeList"></param>
		/// <returns></returns>
		private bool AreFileTypesModifiedInShelvedChangelist(string ShelvedChangelist, List<string> ExtensionTypeList)
		{
			// we don't need to do any of this if there was no extensions typelist passed in
			if (ExtensionTypeList.Count != 0)
			{
				// Get all files in this changelist
				IEnumerable<string> FileList = CommandUtils.P4.Files("@=" + ShelvedChangelist);

				return FileList.Any(F => ExtensionTypeList.Contains(Path.GetExtension(F), StringComparer.OrdinalIgnoreCase));
			}

			return false;
		}

		/// <summary>
		/// Returns true if files with extensions in the provided list are  open locally
		/// </summary>
		/// <param name="ExtensionTypeList"></param>
		/// <returns></returns>
		private bool AreFileTypesModifiedInOpenChangelists(List<string> ExtensionTypeList)
		{
			// we don't need to do any of this if there was no extensions typelist passed in
			if (ExtensionTypeList.Count != 0)
			{
				// Get all files in this changelist
				IEnumerable<string> FileList = CommandUtils.P4.Opened("");

				return FileList.Any(F => ExtensionTypeList.Contains(Path.GetExtension(F), StringComparer.OrdinalIgnoreCase));
			}

			return false;
		}
	}
}
