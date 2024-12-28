function Connect-SQLiteDatabase {
    param (
        [string]$DatabasePath
    )

    # Load SQLite assembly
    Add-Type -Path "S:\SqliteDB\SqliteDotNet\System.Data.SQLite.dll"

    # Create a connection to the SQLite database
    $connectionString = "Data Source=$DatabasePath;Version=3;"
    $connection = New-Object System.Data.SQLite.SQLiteConnection($connectionString)
    $connection.Open()

    return $connection
}

function Check-SHA256InSQLite {
    param (
        [System.Data.SQLite.SQLiteConnection]$Connection,
        [string]$TableName,
        [string]$ColumnName,
        [string]$SHA256Value
    )

    # Create a command to check if the SHA256 value exists
    $query = "SELECT COUNT(*) FROM $TableName WHERE $ColumnName = @SHA256Value"
    $command = $Connection.CreateCommand()
    $command.CommandText = $query
    $command.Parameters.Add((New-Object Data.SQLite.SQLiteParameter("@SHA256Value", $SHA256Value)))

    # Execute the query and get the result
    $result = $command.ExecuteScalar()

    # Return true if the SHA256 value exists, otherwise false
    return [bool]($result -gt 0)
}

function Disconnect-SQLiteDatabase {
    param (
        [System.Data.SQLite.SQLiteConnection]$Connection
    )

    # Close the connection
    $Connection.Close()
}

# Example usage
$databasePath = "S:\SqliteDB\DB\RDS_2024.03.1_modern_minimal.db"
$tableName = "FILE"
$columnName = "sha256"
$sha256Value = "0000008422039A39BA89BB00510BD0F4A121E955BA8895F2E49AB46D2544AFE9"


$connection = Connect-SQLiteDatabase -DatabasePath $databasePath

# do checks here...
$exists = Check-SHA256InSQLite -Connection $connection -TableName $tableName -ColumnName $columnName -SHA256Value $sha256Value

Disconnect-SQLiteDatabase -Connection $connection
