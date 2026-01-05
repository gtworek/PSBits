A Red Team tool designed for Blue Teams? Indeed!

Is this a new development? Yes.

Was advanced research required to create it? No.

Does it circumvent existing security measures? It does not.

Is it superior to other solutions? Not necessarily.

What, then, is the rationale behind its creation?

The goal is to improve detection rules. This tool acts like a true chameleon, nothing about it stays constant or predictable. As a result, detection must focus on the technique rather than the tool itself. This approach can strengthen your defenses, making them tighter and more resilient over time.

I've named it GtExec; I couldn't resist! 

The tool is simply a single DLL. By adding some extra bytes to the end (for example, with `echo yourguid >> gtexec.dll`), you can get around hash-based detection methods. Rename the file to make detections more challenging and you’re ready. You have the flexibility to load it wherever you prefer, using tools like rundll32, regsvr32 etc. Just include the remote machine's name in your command line as the last parameter, and that's when the process truly gets interesting:
- DLL copies itself to remote ADMIN$.
- Svchost-based service is set up.
- Service is started.
 
The service monitors a registry for new commands, executes them, and saves the results back into the registry. The client side then reads this data and displays it.

In essence, this setup allows you to run a remote console as LocalSystem. This isn't a novel concept, so you can now concentrate on your main objective: making your detection rules effective, stopping common lateral movement tactics, and improving your security measures. I’d be genuinely pleased if you accomplish these goals. 

I made every effort to ensure it was safe and took care to clean everything up once you were done using it.



