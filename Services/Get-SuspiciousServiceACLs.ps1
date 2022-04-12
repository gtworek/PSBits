# The script uses WMI to get ACLs for services and identify non-admin DC, WD or WO permissions.
# Any of these permission allows instant elevation to localsystem for any user/group holding it.

# Piece of theory - https://support.microsoft.com/en-us/topic/3cf7240a-86ad-1fc3-bbb6-f468454981c4

# TODO #1: Make it work remotely (easy as we use WMI here)
# TODO #2: Declare suspicious privileges as an array variable instead of hardcoding strings in "if"
# TODO #3: Error checking for resolving SIDs. Aliases resolving.
# TODO #4: Make it return neat PS object instead of bunch of "write-host"

##############
# thanks @kaiserschloss for comments
##############

$DebugPreference = "Continue"
$services = (Get-WmiObject Win32_Service -EnableAllPrivileges)
foreach ($srv in $services)
{
    $sd = ($srv.GetSecurityDescriptor())
    if ($sd.ReturnValue -ne 0)
    {
        Write-Debug ("Service: "+$srv.name+"`tError "+$sd.ReturnValue) -ErrorAction SilentlyContinue
        continue
    }

    $SDDL = ([wmiclass]"win32_SecurityDescriptorHelper").Win32SDtoSDDL($sd.Descriptor).SDDL
    foreach ($ACE in $sddl.split("()"))
    {
        if ($ACE.Split(";")[0] -ne "A")
        {
            continue #as we have non-"allow something" ACE
        }
    
        # we should have set of permissions in the $ACE.Split(";")[2]. 
        # Let's convert it to XX-YY-ZZ- format (in $dashRights) to avoid false positives.
        # the security principal (user/group/etc) in the $ACE.Split(";")[5] ($PrincipalFromSDDL)
        $dashRights = ""
        for ($i = 0; $i -lt (($ACE.Split(";")[2]).Length); $i+=2)
        {
            $dashRights += ($ACE.Split(";")[2]).Substring($i,2) + '-'
        }
        
        $PrincipalFromSDDL = $ACE.Split(";")[5]

        if ( $dashRights.Contains("WD") -or $dashRights.Contains("WO") -or $dashRights.Contains("DC") )
        {
            if ( ($PrincipalFromSDDL -eq "BA") -or ($PrincipalFromSDDL -eq "SY"))# we do not care about local administrators and localsystem as they should have such permissions
            {
                continue
            }
            $PrincipalName = $PrincipalFromSDDL
            if ($PrincipalName.StartsWith("S-1-5-")) #sid and not alias
            {
                $SID = New-Object System.Security.Principal.SecurityIdentifier($PrincipalFromSDDL)
                $PrincipalName = $SID.Translate([System.Security.Principal.NTAccount]).Value
            }
            if ($PrincipalName -eq "NT SERVICE\TrustedInstaller") #ignoring as it is safe
            {
                continue
            }
            if ($PrincipalName -eq ("NT SERVICE\"+$srv.Name)) #ignoring as it is safe
            {
                continue
            }

            Write-Host $srv.Name - $ACE.Split(";")[2] - $PrincipalFromSDDL - $PrincipalName
        } 
    }
} 
