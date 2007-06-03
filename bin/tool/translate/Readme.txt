Readme notes from voidreality:

This is a database translator setup created to translate jAthena databases from Japanese into English, originally created for fAthena, but now being used for Aurora. The translation scripts can be used to translate jAthena's databases into other languages as well quite easily.

The .pl files are Perl scripts which translate jAthena's japanese databases in ./japanese to another language based on the files in ./english. The database files in ./english can be imported from any and all of the up-to-date, mainstream Athena-based emulators, including Aurora, jAthena, eAthena, Freya, Nezumi, cAthena, Cronus, and the rest. Once the files are translated, they are placed in ./output by the converter. The database (./db) folder structure is also converted from jAthena's to Phaeton's.

If you cannot run the .pl files, you don't have the Perl environment active on your computer. You can download and install it via the Perl internet website. To do the full database conversion, simply run 'translate.bat', and the four Perl scripts will be activated and will place the translated files in ./output. Once the files are translated, simply copy + paste them to your emulator's ./db folder and make any necessary adjustments.

Also note before translation, make sure your databases in ./english and ./japanese are fully up-to-date. Report any program bugs to voidreality, aka me.

-> Translation scripts author: Yor (DarkRaven)
-> Contact information: http://ro-freya.net/
