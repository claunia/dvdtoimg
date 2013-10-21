DVDToIMG v1.0.
21 Oct 2013.
Written Natalia Portillo <natalia@claunia.com>
Based on Truman's CDToImg

This little program reads a whole DVD-ROM, dumping Physical Format Information,
Copyright Management Information, Disc Manufacturing Information and Burst
Cutting Area.

This writes the user data as an ISO file (a bunch of decoded sectors) and the
rest of the DVD information as given by the drive, PMI.BIN, CMI.BIN, DMI.BIN
and BCA.BIN respectively.

In a future version this will calculate the hashes (CRC32, MD5 and SHA1) on-the-fly.

This code uses:
- libcdio as to cross-platform-esque send MMC commands to CD/DVD/BD drives.
- The SCSI codes used in this source were taken from MMC6.
- SCSI READ (12) and READ CAPACITY commands.
- MMC READ DISC STRUCTURE command.
- Determine errors, retrieve and decode a few sense data.

This will not work with CD discs. May work with BD discs, not tested, but shouldn't.

You need to have libcdio, development headers, to compile, and binary library to use.

cdtoimg <drive path> <outputfile>


History
-------
v1.0 - 21 Oct 2013
- First release.