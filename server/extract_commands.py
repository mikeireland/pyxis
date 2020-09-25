#!/usr/bin/env python
"""
Extract commands and help strings from the server c++ file and turn them into
the .include files. This ensures that neither help nor commands ever go missing.
"""
import pytomlpp
in_cmd = False
help_dict = {}
with open('server.cpp', 'r') as ff:
    for line in ff.readlines():
        if not in_cmd:
            #Search for Cmd_
            cmd_pos = line.find('Cmd_')
            bracket_pos = line.find('(')
            if (cmd_pos > 0) and (bracket_pos > 0):
                command = line[cmd_pos+4:bracket_pos]
                helpstring = ''
                in_cmd=True
        else:
            slash_pos = line.find('//')
            if (slash_pos<0):
                raise UserWarning("Error: Comment block for command {:s} not terminated with //---".format(command))
            if (line[slash_pos+2:slash_pos+5]=='---'):
                in_cmd=False
                help_dict[command] = helpstring
            else:
                helpstring += line[slash_pos+2:]
                

#All read in. Now lets dump the output for help.
with open('help.toml', 'w') as ff:
    pytomlpp.dump(help_dict, ff)
    
#Now lets go through all the commands and create the .include files
f_elseif = open('cmd_elseif.include', 'w')
f_helpstring = open('helpstring.include', 'w')

#Initial writing...
f_elseif.write('    ')
f_helpstring.write('           const char *kHelpString="')

for ix, cmd in enumerate(help_dict.keys()):
    if ix>0:
        f_elseif.write('    } else ')
    f_elseif.write('if (strcmp(argv[0], "{0:s}") == 0){{\n        return this->Cmd_{0:s}(argc, argv);\n'.format(cmd))
    f_helpstring.write(cmd);
    if ( (ix+1) % 3 ==0 ):
        f_helpstring.write('\\n');
    else:
        f_helpstring.write('\\t');
f_helpstring.write('";\n');
f_elseif.close()
f_helpstring.close()
