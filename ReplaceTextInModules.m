function ReplaceTextInModules(Multiple)

[FileName,PathName] = uigetfile('*.m', 'Choose the M-file with the old text');
if FileName == 0
    return
end
TextToRemove = retrievetextfromfile([PathName,FileName]);
[FileName,PathName] = uigetfile('*.m', 'Choose the M-file with the new text');
if FileName == 0
    return
end
TextToAddInItsPlace = retrievetextfromfile([PathName,FileName]);
PathName = uigetdir(pwd,'Choose the folder in which you want to search and replace')
if PathName == 0
    return
end
cd(PathName)
%%% Retrieves every file in the current directory that begins with
%%% "Alg".
FilesAndDirsStructure = dir(PathName);
FileAndDirNames = sortrows({FilesAndDirsStructure.name}');
LogicalIsDirectory = [FilesAndDirsStructure.isdir];
FileNamesNoDir = FileAndDirNames(~LogicalIsDirectory);
AlgorithmFileNames = FileNamesNoDir(strncmp(FileNamesNoDir,'Alg',3));
NumberOfAlgorithmFiles = size(AlgorithmFileNames,1)
Answer = questdlg('Do you want to replace all instances of the text or just the first?','','All','First','Cancel','All');
if strcmp(Answer,'All') == 1
Multiple = 1;
elseif strcmp(Answer,'First') == 1
    Multiple = 0;
else return   
end
%%% Loops through each Algorithm.
for i = 1:NumberOfAlgorithmFiles
    %%% Opens each file & reads its contents as a string.
    OriginalAlgorithmContents = retrievetextfromfile(cell2mat([AlgorithmFileNames(i,:)]));
    PositionsOfLocatedText = strfind(OriginalAlgorithmContents,TextToRemove);
    if isempty(PositionsOfLocatedText)==1
        %%% If a match was not found, run the following line.
        Result(i,:) = {['NO replacement for ', cell2mat(AlgorithmFileNames(i,:))]};
    else
        if Multiple == 1
            LimitToReplace = length(PositionsOfLocatedText);
        else LimitToReplace = 1;
        end
        NewAlgorithmContents = OriginalAlgorithmContents;
        for j = 1:LimitToReplace
            Number = LimitToReplace+1-j;
            PositionToReplace = PositionsOfLocatedText(Number);
            %%% Piece together the file with the beginning part and the
            %%% ending part and the text to add in the middle.
            PreReplacementText = NewAlgorithmContents(1:PositionToReplace-1);
            PostReplacementText = NewAlgorithmContents(PositionToReplace + length(TextToRemove):end);
            NewAlgorithmContents = [PreReplacementText,TextToAddInItsPlace,PostReplacementText];
            fid=fopen(cell2mat([AlgorithmFileNames(i,:)]),'w');
            fwrite(fid,NewAlgorithmContents,'char');
            fclose(fid);
        end
        Result(i,:) = {[num2str(LimitToReplace),' successful replacement(s) for ', cell2mat(AlgorithmFileNames(i,:))]};
    end
end
%%% Prints the results at the command line.
Result

%%% SUBFUNCTION
function ExtractedText = retrievetextfromfile(PathAndFileName)
%%% Opens the file and retrieves the TextToRemove.
fid=fopen(PathAndFileName);
ExtractedText = char(fread(fid,inf,'char')');
fclose(fid);