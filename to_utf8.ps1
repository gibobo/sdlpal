Get-ChildItem .\sdlpal -Recurse -Include *.cpp |
	Rename-Item -NewName {
		$_.FullName + ".bkup"
	}
Get-ChildItem .\sdlpal -Recurse -Include *.cpp.bkup |
	ForEach-Object {
		Get-Content $_.FullName |
			Out-File -Encoding UTF8 ($_.FullName -replace '.cpp.bkup','.cpp')
			Remove-Item $_.FullName
	}

Get-ChildItem .\sdlpal -Recurse -Include *.c |
	Rename-Item -NewName {
		$_.FullName + ".bkup"
	}
Get-ChildItem .\sdlpal -Recurse -Include *.c.bkup |
	ForEach-Object {
		Get-Content $_.FullName |
			Out-File -Encoding UTF8 ($_.FullName -replace '.c.bkup','.c')
			Remove-Item $_.FullName
	}

Get-ChildItem .\sdlpal -Recurse -Include *.h |
	Rename-Item -NewName {
		$_.FullName + ".bkup"
	}
Get-ChildItem .\sdlpal -Recurse -Include *.h.bkup |
	ForEach-Object {
		Get-Content $_.FullName |
			Out-File -Encoding UTF8 ($_.FullName -replace '.h.bkup','.h')
			Remove-Item $_.FullName
	}

Get-ChildItem .\sdlpal -Recurse -Include *.hpp |
	Rename-Item -NewName {
		$_.FullName + ".bkup"
	}
Get-ChildItem .\sdlpal -Recurse -Include *.hpp.bkup |
	ForEach-Object {
		Get-Content $_.FullName |
			Out-File -Encoding UTF8 ($_.FullName -replace '.hpp.bkup','.hpp')
			Remove-Item $_.FullName
	}
