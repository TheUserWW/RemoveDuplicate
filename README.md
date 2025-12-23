# RemoveDuplicate - Duplicate File Finder and Remover

## Overview
<img width="589" height="450" alt="image" src="https://github.com/user-attachments/assets/4aa55dd2-bfb1-4cab-a2d7-1b5ca2b4fa55" />

RemoveDuplicate is a Windows application designed to help users find and remove duplicate files within folders. It uses MD5 hashing to identify identical files and provides a user-friendly interface to manage and delete duplicates while keeping one copy of each file.

## Features

### Core Functionality
- **Recursive Folder Scanning**: Scan entire directory structures for duplicate files
- **MD5 Hash Verification**: Use cryptographic hashing to ensure accurate duplicate detection
- **Safe Deletion**: Move duplicates to Recycle Bin instead of permanent deletion
- **Grouped Display**: Organize duplicate files into logical groups for easy management
- **Selective Deletion**: Choose which duplicate groups to delete (default: keep first file in each group)
- **Progress Tracking**: Visual progress bars and status updates during scanning and deletion

### Filtering System
- **Size-based Filtering**: Filter files by size range (bytes, KB, MB, GB, TB)
- **Type-based Filtering**: Filter by file extensions (e.g., *.txt, *.jpg)
- **Date-based Filtering**: Filter by file modification date range
- **Path-based Filtering**: Filter by partial path matching
- **Combined Filters**: Apply multiple filters simultaneously
- **Filter Persistence**: Filters are saved and restored between sessions

## Installation

### Requirements
- Windows 7 or later (32-bit or 64-bit)
- Visual Studio 2022 with C++ support (for building from source)
- Minimum 4GB RAM recommended for scanning large folders

### Building from Source
1. Open `RemoveDuplicate.sln` in Visual Studio 2022
2. Select desired configuration (Debug/Release, x86/x64)
3. Build the solution (Build → Build Solution)
4. Executable will be created in the corresponding output directory

## Usage Guide

### Basic Operations

#### 1. Selecting a Folder
- Click the **Browse** button or use the "Folder Path" edit box
- Navigate to the folder you want to scan for duplicates
- The selected path will appear in the edit box

#### 2. Scanning for Duplicates
- Click the **Scan Duplicates** button
- The application will:
  - Recursively scan all files in the selected folder
  - Calculate MD5 hashes for each file
  - Group identical files together
  - Display results in the list view

#### 3. Managing Duplicate Groups
- Each duplicate group shows all identical files
- Files are automatically checked for deletion (except the first file in each group)
- Use checkboxes to select/deselect individual files or groups
- **Select All**: Check all groups for deletion
- **Deselect All**: Uncheck all groups

#### 4. Deleting Duplicates
- Click the **Delete Duplicates** button
- Confirm the deletion when prompted
- Selected duplicate files will be moved to the Recycle Bin
- One copy of each file will be preserved

### Filtering Options - Detailed Guide
<img width="395" height="391" alt="image" src="https://github.com/user-attachments/assets/ca7ff7a8-d913-4c04-8e56-7ecbffe2aebe" />

The filtering system allows you to narrow down which duplicate files are displayed and managed. Filters can be accessed through the **Filter** menu or filter settings dialog.

#### Accessing Filter Settings
1. **Menu Method**: Click **Filter → Filter Settings** from the main menu
2. **Dialog Method**: The filter settings dialog provides access to all filter options

#### Filter Types and Configuration

##### 1. Size Filter
**Purpose**: Filter files based on file size range

**Configuration Options**:
- **Minimum Size**: Enter the smallest file size to include
- **Maximum Size**: Enter the largest file size to include
- **Size Format Support**:
  - Bytes: `1024`, `1KB`, `2MB`, `0.5GB`, `1TB`
  - Unit abbreviations: `B`, `KB`, `MB`, `GB`, `TB`
  - Examples: 
    - `10KB` = 10 kilobytes
    - `2.5MB` = 2.5 megabytes
    - `1GB` = 1 gigabyte

**Usage Examples**:
- Find duplicates larger than 1MB: Set Min = `1MB`, Max = (empty)
- Find duplicates between 100KB and 5MB: Set Min = `100KB`, Max = `5MB`
- Find small duplicates: Set Min = `0`, Max = `100KB`

##### 2. Type Filter
**Purpose**: Filter files by extension or file type

**Configuration Options**:
- **File Types**: Enter file extensions to include
- **Multiple Types**: Separate multiple extensions with semicolons
- **Format Support**:
  - With dots: `.txt`, `.jpg`, `.pdf`
  - Without dots: `txt`, `jpg`, `pdf`
  - With wildcards: `*.txt`, `*.jpg`
  - Mixed: `.txt;*.jpg;pdf`

**Usage Examples**:
- Image files only: `.jpg;.png;.gif;.bmp`
- Document files: `.pdf;.doc;.docx;.txt`
- Executables: `.exe;.msi;.bat`
- Custom combination: `*.txt;*.log;.ini`

##### 3. Date Filter
**Purpose**: Filter files by last modification date

**Configuration Options**:
- **From Date**: Start date for the range (inclusive)
- **To Date**: End date for the range (inclusive)
- **Date Format Support**:
  - YYYY-MM-DD: `2024-01-15`
  - YYYY/MM/DD: `2024/01/15`
  - The "To Date" automatically includes the entire day (up to 23:59:59)

**Usage Examples**:
- Files modified in January 2024: From = `2024-01-01`, To = `2024-01-31`
- Files modified after specific date: From = `2024-01-01`, To = (empty)
- Files modified before specific date: From = (empty), To = `2023-12-31`
- Specific date range: From = `2023-06-01`, To = `2023-08-31`

##### 4. Path Filter
**Purpose**: Filter files by partial path matching

**Configuration Options**:
- **Path Contains**: Enter text that must appear in the file path
- **Case Insensitive**: Matching is not case-sensitive
- **Partial Matching**: Matches any part of the full path

**Usage Examples**:
- Files in "Downloads" folder: `Downloads`
- Files with specific username: `\Users\John\`
- Files in temporary folders: `temp` or `tmp`
- Specific project files: `\ProjectX\`

#### Advanced Filter Combinations

Filters can be combined for precise control:

1. **AND Logic**: All enabled filters must match for a file to be displayed
2. **Order of Evaluation**: Size → Type → Date → Path
3. **Partial Filters**: You can enable a filter without setting values (acts as wildcard)

**Complex Examples**:
- Find large image files modified recently:
  - Size: Min = `1MB`, Max = (empty)
  - Type: `.jpg;.png;.bmp`
  - Date: From = `2024-01-01`
  - Path: (empty)

- Find old text documents in specific folder:
  - Size: (any)
  - Type: `.txt;.doc`
  - Date: To = `2020-12-31`
  - Path: `\Archive\`

#### Managing Filters

##### Applying Filters
1. Configure filters in the Filter Settings dialog
2. Click **OK** to apply filters immediately
3. The list view will update to show only matching duplicate files
4. Status bar shows how many files match the current filters

##### Resetting Filters
- **Menu Method**: Click **Filter → Reset Filters**
- **Dialog Method**: Clear all fields in Filter Settings dialog
- **Effect**: All filters are disabled and all duplicate files are shown

##### Filter Persistence
- Filters are automatically saved when the application closes
- Saved filters are restored when the application starts
- To start fresh, use **Reset Filters**

## Menu Options

### Filter Menu
- **Filter Settings**: Open the comprehensive filter configuration dialog
- **Reset Filters**: Clear all filters and show all duplicate files

### Help Menu
- **About**: Display application information, version, and copyright

## Technical Details

### File Hashing
- Algorithm: MD5 (Message Digest Algorithm 5)
- Buffer size: 4096 bytes per read operation
- Error handling: Files that cannot be hashed are skipped

### Deletion Process
- Method: Windows Shell File Operation (SHFileOperation)
- Destination: Windows Recycle Bin
- Flags: FOF_ALLOWUNDO, FOF_NOCONFIRMATION, FOF_NOERRORUI
- Safety: Always keeps the first file in each duplicate group

### Performance Considerations
- **Memory Usage**: Stores file paths and MD5 hashes in memory during scanning
- **Disk I/O**: Sequential file reading with 4KB buffers
- **CPU Usage**: MD5 calculation is CPU-intensive; consider scanning smaller folders on older systems
- **Progress Updates**: Status updates every 10 files to minimize UI overhead

## Troubleshooting

### Common Issues

#### 1. "No files found" after scanning
- Verify the selected folder contains files
- Check folder permissions
- Ensure filters aren't excluding all files

#### 2. Slow scanning performance
- Limit folder size (scan subfolders separately)
- Close other applications
- Consider using more specific filters

#### 3. Files not being detected as duplicates
- Files must have identical MD5 hashes
- Even one byte difference will prevent matching
- Check for hidden characters or metadata differences

#### 4. Filter not working as expected
- Verify date formats (YYYY-MM-DD)
- Check size units (KB vs MB)
- Ensure type filters include the dot (`.txt` not `txt`)
- Try simpler filters first, then combine

### Error Messages

- **"Cannot calculate hash"**: File may be in use or inaccessible
- **"Cannot delete file"**: File may be read-only or in use
- **"Invalid filter value"**: Check format of size/date inputs

## Best Practices

1. **Start Small**: Test with a small folder before scanning large directories
2. **Use Filters**: Apply filters to focus on specific file types or sizes
3. **Review Before Deleting**: Always check which files are selected for deletion
4. **Backup Important Files**: Ensure you have backups before mass deletion
5. **Use Recycle Bin**: The application uses Recycle Bin, but emptying it is permanent

## Limitations

1. **File System**: Only works with local NTFS/FAT file systems
2. **File Size**: Extremely large files (>4GB) may cause performance issues
3. **Network Paths**: Limited support for network locations
4. **Symbolic Links**: Treats symbolic links as separate files
5. **Encrypted Files**: May not be able to hash encrypted files without proper permissions

## Keyboard Shortcuts

- **F5**: Rescan current folder
- **Ctrl+A**: Select all groups
- **Ctrl+D**: Deselect all groups
- **Enter**: Open selected file
- **Delete**: Delete selected duplicates (with confirmation)
