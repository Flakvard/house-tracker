# Define the root directory for the project
$rootDir = Get-Location

# Get the first-level directories in the root folder
$folders = Get-ChildItem -Path $rootDir -Directory

# Initialize total line and character counts and folder-based summaries
$totalLines = 0
$totalCharacters = 0
$folderLineCounts = @{}
$folderCharacterCounts = @{}
$fileTypes = @("cpp", "hpp", "bat")
$fileTypeLineCounts = @{}
$fileTypeCharacterCounts = @{}

# Initialize line and character counts for each file type
foreach ($fileType in $fileTypes) {
    $fileTypeLineCounts[$fileType] = 0
    $fileTypeCharacterCounts[$fileType] = 0
}

# Function to count lines and characters in a collection of files
function Count-LinesAndCharactersInFiles($files) {
    $lineCount = 0
    $characterCount = 0
    foreach ($file in $files) {
        $content = Get-Content $file.FullName -Raw
        $lines = $content -split "`r`n|`n|`r"
        $lineCount += $lines.Count
        $characterCount += $content.Length
    }
    return @($lineCount, $characterCount)
}

# Count lines and characters in specified file types in the root directory (direct files not in subfolders)
foreach ($fileType in $fileTypes) {
    $filesRoot = Get-ChildItem -Path $rootDir -Filter *.$fileType
    if ($filesRoot) {
        $resultRoot = Count-LinesAndCharactersInFiles($filesRoot)
        $linesRoot = $resultRoot[0]
        $charactersRoot = $resultRoot[1]
        if ($folderLineCounts["Root"]) {
            $folderLineCounts["Root"] += $linesRoot
            $folderCharacterCounts["Root"] += $charactersRoot
        } else {
            $folderLineCounts["Root"] = $linesRoot
            $folderCharacterCounts["Root"] = $charactersRoot
        }
        $fileTypeLineCounts[$fileType] += $linesRoot
        $fileTypeCharacterCounts[$fileType] += $charactersRoot
        $totalLines += $linesRoot
        $totalCharacters += $charactersRoot
    }
}

# Loop through each first-level folder
foreach ($folder in $folders) {
    # Skip bin, obj, and node_modules folders
    if ($folder.Name -in @("bin", "obj", "node_modules", "Migrations")) {
        continue
    }

    foreach ($fileType in $fileTypes) {
        # Recursively get all files of the current type within the folder and all its subdirectories
        $files = Get-ChildItem -Path $folder.FullName -Recurse -Include *.$fileType | Where-Object { 
            $_.FullName -notmatch '\\bin\\|\\obj\\|\\node_modules\\|\\Migrations\\'
        }
        
        # If the folder contains files of the current type, count the lines and characters
        if ($files) {
            $resultFolder = Count-LinesAndCharactersInFiles($files)
            $folderLines = $resultFolder[0]
            $folderCharacters = $resultFolder[1]
            if ($folderLineCounts[$folder.Name]) {
                $folderLineCounts[$folder.Name] += $folderLines
                $folderCharacterCounts[$folder.Name] += $folderCharacters
            } else {
                $folderLineCounts[$folder.Name] = $folderLines
                $folderCharacterCounts[$folder.Name] = $folderCharacters
            }
            $fileTypeLineCounts[$fileType] += $folderLines
            $fileTypeCharacterCounts[$fileType] += $folderCharacters
            $totalLines += $folderLines
            $totalCharacters += $folderCharacters
        }
    }
}

# Remove double-counting of root files (subtract root lines and characters from total)
$totalLines -= $folderLineCounts["Root"]
$totalCharacters -= $folderCharacterCounts["Root"]

# Sort folder line and character counts by the number of lines in descending order
$sortedFolders = $folderLineCounts.GetEnumerator() | Sort-Object Value -Descending

# Output the sorted number of lines and folder-based counts with percentage distribution
Write-Host "`nLines and characters by folder:" -ForegroundColor Cyan
Write-Host "--------------------------------------------------------------------------------------------------------------------------------------"

# Print the header of the table
$headerFormat = "{0,-40} {1,-20} {2,-15} {3,-20} {4,-15}"
Write-Host ($headerFormat -f "Folder", "Lines of Code", "Percentage", "Characters", "Percentage")
Write-Host "--------------------------------------------------------------------------------------------------------------------------------------"

# Print the sorted folder statistics
foreach ($entry in $sortedFolders) {
    $folderName = $entry.Key
    $folderLines = $entry.Value
    $folderCharacters = $folderCharacterCounts[$folderName]
    $percentageLines = [math]::Round(($folderLines / $totalLines) * 100, 2)
    $percentageCharacters = [math]::Round(($folderCharacters / $totalCharacters) * 100, 2)

    # Output folder name in green and lines of code, characters, and percentages in normal color
    Write-Host -NoNewline ("`t{0,-40}" -f $folderName) -ForegroundColor Green
    Write-Host -NoNewline ("{0,-20}" -f $folderLines)
    Write-Host -NoNewline ("{0,-15}%" -f $percentageLines)
    Write-Host -NoNewline ("{0,-20}" -f $folderCharacters)
    Write-Host ("{0,-5}%" -f $percentageCharacters)
}

# Output the total number of lines and characters for each file type
Write-Host "`n--------------------------------------------------------------------------------------------------------------------------------------"
Write-Host ($headerFormat -f "File Type", "Lines of Code", "Percentage", "Characters", "Percentage")
Write-Host "--------------------------------------------------------------------------------------------------------------------------------------"

foreach ($fileType in $fileTypes) {
    $fileTypeLines = $fileTypeLineCounts[$fileType]
    $fileTypeCharacters = $fileTypeCharacterCounts[$fileType]
    $percentageLines = [math]::Round(($fileTypeLines / $totalLines) * 100, 2)
    $percentageCharacters = [math]::Round(($fileTypeCharacters / $totalCharacters) * 100, 2)

    # Output file type in green and lines of code, characters, and percentages in normal color
    Write-Host -NoNewline ("`t{0,-40}" -f ".$fileType") -ForegroundColor Yellow
    Write-Host -NoNewline ("{0,-20}" -f $fileTypeLines)
    Write-Host -NoNewline ("{0,-15}%" -f $percentageLines)
    Write-Host -NoNewline ("{0,-20}" -f $fileTypeCharacters)
    Write-Host ("{0,-5}%" -f $percentageCharacters)
}

Write-Host "`n--------------------------------------------------------------------------------------------------------------------------------------"
Write-Host "`tTotal lines of code in all files: $totalLines" -ForegroundColor Yellow
Write-Host "`tTotal characters in all files: $totalCharacters" -ForegroundColor Yellow
