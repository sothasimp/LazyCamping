using LazyCamping.PluginBuilder;

// Checks LazyCamping.esp's script properties against the .psc sources: every
// property set in the plugin must be declared in its script, and properties
// left empty are listed. LazyCamping.esp itself is edited in the Creation Kit.
//
// Usage: dotnet run --project Plugin -- <LazyCamping.esp> <Scripts/Source>
if (args.Length != 2)
{
    Console.Error.WriteLine("Usage: dotnet run --project Plugin -- <LazyCamping.esp> <Scripts/Source>");
    return 2;
}

try
{
    return ScriptCheck.Run(args[0], args[1]) ? 0 : 1;
}
catch (Exception e)
{
    Console.Error.WriteLine(e);
    return 1;
}
