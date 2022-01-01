I guess, you have a chance to see processes running with an identity not being any real user. For example: DWM-1, NT SERVICE\..., DefaultAppPool, etc. 
Seeing this, I have started to ask myself: can I start my own process with arbitrary "identity" visible for the system? Is it somehow limited?

And I started to analyze available documentation and test my ideas with some code.

Here you can see the result named `RunAsVA`. I am creating (and registering aka mapping) new identity (see the line 239, 240). I am creating couple of SIDs, and then I tell LSA to add a mapping. 

Next step relies on creating a token for such user (line 314).

As the token is not primary, I had to duplicate it (#330).

Tnek I can use the new, primary token to create a new process (#355).

And that's it.

The code is totally harmless, but it has no cleanup options. If you really want to unmap the virtual user, your OS must be restarted. You can live wiht such mapping forever, but wanted to warn you in advance.

As such LSA/token games are destroying the trust to users, tokens, etc., the SeTcbPrivilege is required. The easiest way of having it, is to start the code with LOCALSYSTEM. `psexec.exe -i -s -d cmd.exe` will do the job for you.

Enjoy!

BTW, the most useful source of knowledge, was the Microsoft patent describing how IIS impersonates worker processes. You can see it at: https://patents.justia.com/patent/8640215 

And couple of moments later I have realized what superpower I've got. I can put ANYTHING into the valid token. Did you ever wanted to be TrustedInstaller? Or it is just me...? Grab your copy of `TrustedInstallerCmd` and have fun too! :) 

The `TrustedInstallerCmd2` does not require psexec anymore and it is less talkative. 
