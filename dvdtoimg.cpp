/*
DVDtoIMG v1.00.
21 Oct 2013.
Written by Natalia Portillo <natalia@claunia.com>
Based on Truman's CDtoIMG

Reads an entire DVD, dumping the Physical Format Information, Copyright Management Information,
Disc Manufacturing Information and Burst Cutting Area.
*/

#include <stdio.h>
#include <malloc.h>

// CDIO
#include <cdio/cdio.h>
#include <cdio/mmc.h>

#include <string.h> // For memset()
#include <stdlib.h> // For atoi()

//Global variables..
unsigned char *data_buf;  //Buffer for holding transfer data from or to drive.

// Opens device using libcdio
CdIo_t *open_volume(char *drive_letter)
{
	return cdio_open (drive_letter, DRIVER_DEVICE);
}

/* Displays sense error information. */
void disp_sense(CdIo_t *p_cdio)
{
    cdio_mmc_request_sense_t *pp_sense;
    
    int cmd_ret;
    
    cmd_ret =  mmc_last_cmd_sense(p_cdio, &pp_sense);
    
    if(cmd_ret < 0)
    	printf(" - Error reading last MMC sense.");
    else if(cmd_ret == 0)
    	printf(" - No additional sense info.");
    else
    {
	    printf("Sense data, key:ASC:ASCQ: %02X:%02X:%02X", pp_sense->sense_key, pp_sense->asc, pp_sense->ascq);

	    //Decode sense key:ASC:ASCQ.
	    //It's a very short list - I'm just trying to show you how to decode into text.
	    //You really need to look into MMC document and change this into an exhaustive list from
	    //the sense error table that is found in there.
	    if(pp_sense->sense_key==CDIO_MMC_SENSE_KEY_NO_SENSE)
	    {
	        if(pp_sense->asc==0x00)
	        {
	            if(pp_sense->ascq==0x00)
	            {
	                printf(" - No additional sense info.");  //No errors
	            }
	        }
	    }
	    else
	    if(pp_sense->sense_key==CDIO_MMC_SENSE_KEY_NOT_READY)
	    {
    	    if(pp_sense->asc==0x3A)
	        {
        	    if(pp_sense->ascq==0x00)
            	{
    	            printf(" - Medium not present.");
	            }
            	else
        	    if(pp_sense->ascq==0x01)
    	        {
	                printf(" - Medium not present-tray closed.");
            	}
        	    else
    	        if(pp_sense->ascq==0x02)
	            {
            	    printf(" - Medium not present-tray open.");
        	    }
    	    }
	    }
    }
    
    printf("\n");
}

/*
    1. Set up the sptd values.
    2. Set up the CDB for SPC1 test unit ready command.
    3. Send the request to the drive.
*/
driver_return_code_t test_unit_ready(CdIo_t *p_cdio)
{
    mmc_cdb_t cdb = {{0, }};

    //CDB with values for Test Unit Ready CDB6 command.
    //The values were taken from SPC1 draft paper.
    cdb.field[0]=0x00;  //Code for Test Unit Ready CDB6 command.
    cdb.field[1]=0;
    cdb.field[2]=0;
    cdb.field[3]=0;
    cdb.field[4]=0;
    cdb.field[5]=0;
    cdb.field[6]=0;
    cdb.field[7]=0;
    cdb.field[8]=0;
    cdb.field[9]=0;
    cdb.field[10]=0;
    cdb.field[11]=0;
    cdb.field[12]=0;
    cdb.field[13]=0;
    cdb.field[14]=0;
    cdb.field[15]=0;

	return mmc_run_cmd(p_cdio, 108000000, &cdb, SCSI_MMC_DATA_NONE, 0, NULL);
}

// Reads DVD PFI using READ DVD STRUCTURE command
driver_return_code_t read_PFI(CdIo_t *p_cdio,
					   unsigned short int in_data_trans_len,
                       unsigned int layer)
{
    mmc_cdb_t cdb = {{0, }};

    //CDB with values for READ DVD STRUCTURE CDB12 command.
    cdb.field[0]=0xAD;  //Code for READ DVD STRUCTURE CDB12 command.
    cdb.field[1]=0; // Obsolete
    cdb.field[2]=0; // LBA byte 0
    cdb.field[3]=0; // LBA byte 1
    cdb.field[4]=0; // LBA byte 2
    cdb.field[5]=0; // LBA byte 3
    cdb.field[6]=(unsigned char)layer;
    cdb.field[7]=0; // Format: PFI
    cdb.field[8]=(unsigned char)(in_data_trans_len >> 8);  //MSB of max length of bytes to receive.
    cdb.field[9]=(unsigned char)in_data_trans_len;  //LSB of max length of bytes to receive.
    cdb.field[10]=0;
    cdb.field[11]=0;
    cdb.field[12]=0;
    cdb.field[13]=0;
    cdb.field[14]=0;
    cdb.field[15]=0;

    memset(data_buf, 0, in_data_trans_len);

	return mmc_run_cmd(p_cdio, 108000000, &cdb, SCSI_MMC_DATA_READ, in_data_trans_len, (void *)data_buf);
}

// Gets DVD size using READ CAPACITY command
driver_return_code_t read_CAPACITY(CdIo_t *p_cdio)
{
    mmc_cdb_t cdb = {{0, }};

    //CDB with values for READ CAPACITY command.
    cdb.field[0]=0x25;  //Code for READ CAPACITY CDB10 command.
    cdb.field[1]=0; // Obsolete
    cdb.field[2]=0; // LBA byte 0
    cdb.field[3]=0; // LBA byte 1
    cdb.field[4]=0; // LBA byte 2
    cdb.field[5]=0; // LBA byte 3
    cdb.field[6]=0; // Reserved
    cdb.field[7]=0; // Reserved
    cdb.field[8]=0; // PMI
    cdb.field[9]=0; // Control
    cdb.field[10]=0;
    cdb.field[11]=0;
    cdb.field[12]=0;
    cdb.field[13]=0;
    cdb.field[14]=0;
    cdb.field[15]=0;

    memset(data_buf, 0, 8);

	return mmc_run_cmd(p_cdio, 108000000, &cdb, SCSI_MMC_DATA_READ, 8, (void *)data_buf);
}

// Reads DVD CMI using READ DVD STRUCTURE command
driver_return_code_t read_CMI(CdIo_t *p_cdio,
					   unsigned short int in_data_trans_len,
                       unsigned char layer)
{
    mmc_cdb_t cdb = {{0, }};

    //CDB with values for READ DVD STRUCTURE CDB12 command.
    cdb.field[0]=0xAD;  //Code for READ DVD STRUCTURE CDB12 command.
    cdb.field[1]=0; // Obsolete
    cdb.field[2]=0; // LBA byte 0
    cdb.field[3]=0; // LBA byte 1
    cdb.field[4]=0; // LBA byte 2
    cdb.field[5]=0; // LBA byte 3
    cdb.field[6]=layer;
    cdb.field[7]=1; // Format: Copyright information
    cdb.field[8]=(unsigned char)(in_data_trans_len >> 8);  //MSB of max length of bytes to receive.
    cdb.field[9]=(unsigned char)in_data_trans_len;  //LSB of max length of bytes to receive.
    cdb.field[10]=0;
    cdb.field[11]=0;
    cdb.field[12]=0;
    cdb.field[13]=0;
    cdb.field[14]=0;
    cdb.field[15]=0;

    memset(data_buf, 0, in_data_trans_len);

	return mmc_run_cmd(p_cdio, 108000000, &cdb, SCSI_MMC_DATA_READ, in_data_trans_len, (void *)data_buf);
}

// Reads DVD BCA using READ DVD STRUCTURE command
driver_return_code_t read_BCA(CdIo_t *p_cdio,
					   unsigned short int in_data_trans_len)
{
    mmc_cdb_t cdb = {{0, }};

    //CDB with values for READ DVD STRUCTURE CDB12 command.
    cdb.field[0]=0xAD;  //Code for READ DVD STRUCTURE CDB12 command.
    cdb.field[1]=0; // Obsolete
    cdb.field[2]=0; // LBA byte 0
    cdb.field[3]=0; // LBA byte 1
    cdb.field[4]=0; // LBA byte 2
    cdb.field[5]=0; // LBA byte 3
    cdb.field[6]=0; // For reading BCA, layer should be always set as 0
    cdb.field[7]=3; // Format: Burst cutting area
    cdb.field[8]=(unsigned char)(in_data_trans_len >> 8);  //MSB of max length of bytes to receive.
    cdb.field[9]=(unsigned char)in_data_trans_len;  //LSB of max length of bytes to receive.
    cdb.field[10]=0;
    cdb.field[11]=0;
    cdb.field[12]=0;
    cdb.field[13]=0;
    cdb.field[14]=0;
    cdb.field[15]=0;

    memset(data_buf, 0, in_data_trans_len);

	return mmc_run_cmd(p_cdio, 108000000, &cdb, SCSI_MMC_DATA_READ, in_data_trans_len, (void *)data_buf);
}

// Reads DVD DMI using READ DVD STRUCTURE command
driver_return_code_t read_DMI(CdIo_t *p_cdio,
					   unsigned short int in_data_trans_len,
                       unsigned char layer)
{
    mmc_cdb_t cdb = {{0, }};

    //CDB with values for READ DVD STRUCTURE CDB12 command.
    cdb.field[0]=0xAD;  //Code for READ DVD STRUCTURE CDB12 command.
    cdb.field[1]=0; // Obsolete
    cdb.field[2]=0; // LBA byte 0
    cdb.field[3]=0; // LBA byte 1
    cdb.field[4]=0; // LBA byte 2
    cdb.field[5]=0; // LBA byte 3
    cdb.field[6]=layer;
    cdb.field[7]=4; // Format: DMI
    cdb.field[8]=(unsigned char)(in_data_trans_len >> 8);  //MSB of max length of bytes to receive.
    cdb.field[9]=(unsigned char)in_data_trans_len;  //LSB of max length of bytes to receive.
    cdb.field[10]=0;
    cdb.field[11]=0;
    cdb.field[12]=0;
    cdb.field[13]=0;
    cdb.field[14]=0;
    cdb.field[15]=0;

    memset(data_buf, 0, in_data_trans_len);

	return mmc_run_cmd(p_cdio, 108000000, &cdb, SCSI_MMC_DATA_READ, in_data_trans_len, (void *)data_buf);
}

// Reads sectors using READ (CDB12) command
driver_return_code_t read_12(CdIo_t *p_cdio,
                  long int MMC_LBA_sector,
                  unsigned long int n_sectors)
{
    mmc_cdb_t cdb = {{0, }};
    long int MMC_LBA_sector2;
    unsigned long int n_sectors2;

    cdb.field[0]=0x28;  //Code for Read 12 command.
    cdb.field[1]=0x8; //Force Unit Access, no cached data
    
    //Fill in starting MMC sector (CDB[2] to CDB[5])..
    cdb.field[5]=(unsigned char)MMC_LBA_sector;   //Least sig byte of LBA sector no. to read from CD.
    MMC_LBA_sector2=MMC_LBA_sector>>8;
    cdb.field[4]=(unsigned char)MMC_LBA_sector2;  //2nd byte.
    MMC_LBA_sector2=MMC_LBA_sector2>>8;
    cdb.field[3]=(unsigned char)MMC_LBA_sector2;  //3rd byte.
    MMC_LBA_sector2=MMC_LBA_sector2>>8;
    cdb.field[2]=(unsigned char)MMC_LBA_sector2;  //Most significant byte.

    //Fill in no. of sectors to read (CDB[6] to CDB[8])..
    cdb.field[8]=(unsigned char)n_sectors;  //No. of sectors to read from CD byte 0 (LSB).
    n_sectors2=n_sectors>>8;
    cdb.field[7]=(unsigned char)n_sectors2;  //No. of sectors to read from CD byte 1.
    n_sectors2=n_sectors2>>8;
    cdb.field[6]=(unsigned char)n_sectors2;  //No. of sectors to read from CD byte 2 (MSB).
    cdb.field[9]=0;
    
    cdb.field[10]=0;  // No streaming
    cdb.field[11]=0;
    cdb.field[12]=0;
    cdb.field[13]=0;
    cdb.field[14]=0;
    cdb.field[15]=0;

    memset(data_buf, 0, 2048*n_sectors);

	return mmc_run_cmd(p_cdio, 108000000, &cdb, SCSI_MMC_DATA_READ, 2048*n_sectors, (void *)data_buf);    
}

driver_return_code_t verified_read_PFI_layer(CdIo_t *p_cdio,
                       unsigned long int data_buffer_size, unsigned int layer)
{
    driver_return_code_t success;
    unsigned short int alloc_len=0;
    unsigned char *pfi_buf;
    char *pfi_name;
   	FILE *file_ptr;

    printf("Sending read PFI command..");
    //Sends MMC1 READ TOC/PMA/ATIP command to drive to get 4 byte header.
    success=read_PFI(p_cdio, 65535, 0);
   	
    if(success==DRIVER_OP_SUCCESS)
    {
        alloc_len=data_buf[0] << 8;
        alloc_len=alloc_len | data_buf[1];
    }
    else
    {
        printf("done.\n");
        if(success==DRIVER_OP_MMC_SENSE_DATA)
        {
            disp_sense(p_cdio);
        }
        else
        {
            printf("Command sent but returned with an unhandled status code: %02X\n", success);
        }
    }

    if(success==DRIVER_OP_SUCCESS && (alloc_len>0))
    {
        //Limit alloc len to maximum allowed by size of data transfer buffer length.
        if((alloc_len+2)>data_buffer_size)
        {
            alloc_len=data_buffer_size-2;
        }

        //Sends MMC1 READ TOC/PMA/ATIP command to drive to get full data.
        success=read_PFI(p_cdio, alloc_len+4, layer);
        printf("done.\n");
        if(success==DRIVER_OP_SUCCESS)
        {
            alloc_len=data_buf[0] << 8;
            alloc_len=alloc_len | data_buf[1];
            alloc_len-=2;
            
            pfi_buf = data_buf+4;
            
            printf("Saving layer %d PFI to PFI%d.BIN\n", layer, layer);
            sprintf(pfi_name, "PFI%d.BIN", layer);
            file_ptr=fopen(pfi_name, "wb");
            fwrite(pfi_buf, alloc_len, 1, file_ptr);
            fclose(file_ptr);
        }
        else
        {
            if(success==DRIVER_OP_MMC_SENSE_DATA)
            {
                disp_sense(p_cdio);
            }
            else
            {
                printf("Command sent but returned with an unhandled status code: %02X\n", success);
            }
        }
    }
    else
    {
        printf("failed.\n");
    }

    return success;
}

// Reads and dumps DVD Burst Cutting Area
driver_return_code_t verified_read_BCA(CdIo_t *p_cdio,
                       unsigned long int data_buffer_size)
{
    driver_return_code_t success;
    unsigned short int alloc_len=0;
    unsigned char *bca_buf;
   	FILE *file_ptr;

    printf("Sending read BCA command..");
    //Sends MMC1 READ TOC/PMA/ATIP command to drive to get 4 byte header.
    success=read_BCA(p_cdio, 65535);
   	
    if(success==DRIVER_OP_SUCCESS)
    {
        alloc_len=data_buf[0] << 8;
        alloc_len=alloc_len | data_buf[1];
    }
    else
    {
        printf("done.\n");
        if(success==DRIVER_OP_MMC_SENSE_DATA)
        {
            disp_sense(p_cdio);
        }
        else
        {
            printf("Command sent but returned with an unhandled status code: %02X\n", success);
        }
    }

    if(success==DRIVER_OP_SUCCESS && (alloc_len>0))
    {
        //Limit alloc len to maximum allowed by size of data transfer buffer length.
        if((alloc_len+2)>data_buffer_size)
        {
            alloc_len=data_buffer_size-2;
        }

        success=read_BCA(p_cdio, alloc_len+4);
        printf("done.\n");
        if(success==DRIVER_OP_SUCCESS)
        {
            alloc_len=data_buf[0] << 8;
            alloc_len=alloc_len | data_buf[1];
            alloc_len-=2;
            
            bca_buf=data_buf+4;
        	printf("Saving Burst Cutting Area as BCA.BIN\n");
       		file_ptr=fopen("BCA.BIN", "wb");
       		fwrite(bca_buf, alloc_len, 1, file_ptr);
       		fclose(file_ptr);
        }
        else
        {
            if(success==DRIVER_OP_MMC_SENSE_DATA)
            {
                disp_sense(p_cdio);
            }
            else
            {
                printf("Command sent but returned with an unhandled status code: %02X\n", success);
            }
        }
    }
    else
    {
        printf("failed.\n");
        printf("Unable to read Burst Cutting Area.\n");
    }

    return success;
}

// Reads the DVD PFI, checks for PTP or OTP, dumps all the applicable PFIs.
driver_return_code_t verified_read_PFI(CdIo_t *p_cdio,
                       unsigned long int data_buffer_size)
{
    driver_return_code_t success;
    unsigned short int alloc_len=0;
   	unsigned char *pfi_buf;
   	unsigned short int category;
   	char *category_str;
   	unsigned short int layers;
   	unsigned short int track_path;
   	FILE *file_ptr;
   	int has_bca;

    printf("Sending read PFI command..");
    //Sends MMC1 READ TOC/PMA/ATIP command to drive to get 4 byte header.
    success=read_PFI(p_cdio, 65535, 0);
   	
    if(success==DRIVER_OP_SUCCESS)
    {
        alloc_len=data_buf[0] << 8;
        alloc_len=alloc_len | data_buf[1];
    }
    else
    {
        printf("done.\n");
        if(success==DRIVER_OP_MMC_SENSE_DATA)
        {
            disp_sense(p_cdio);
        }
        else
        {
            printf("Command sent but returned with an unhandled status code: %02X\n", success);
        }
    }

    if(success==DRIVER_OP_SUCCESS && (alloc_len>0))
    {
        //Limit alloc len to maximum allowed by size of data transfer buffer length.
        if((alloc_len+2)>data_buffer_size)
        {
            alloc_len=data_buffer_size-2;
        }

        success=read_PFI(p_cdio, alloc_len+4, 0);
        printf("done.\n");
        if(success==DRIVER_OP_SUCCESS)
        {
            alloc_len=data_buf[0] << 8;
            alloc_len=alloc_len | data_buf[1];
            alloc_len-=2;
            
			pfi_buf = data_buf+4; // Set PFI buffer omitting PFI size fields
			category = pfi_buf[0] >> 4;
			switch(category)
			{
				case 0:
					category_str="DVD-ROM";
					break;
				case 1:
					category_str="DVD-RAM";
					break;
				case 2:
					category_str="DVD-R";
					break;
				case 3:
					category_str="DVD-RW";
					break;
				case 4:
					category_str="HD DVD-ROM";
					break;
				case 5:
					category_str="HD DVD-RAM";
					break;
				case 6:
					category_str="HD DVD-RW";
					break;
				case 9:
					category_str="DVD+RW";
					break;
				case 10:
					category_str="DVD+R";
					break;
				case 13:
					category_str="DVD+RW DL";
					break;
				case 14:
					category_str="DVD+R DL";
					break;
				default:
					category_str="Unknown";
					break;
			}
			printf("Disk is type %s, version %d\n", category_str, pfi_buf[0]&0xF);
			
			layers=(pfi_buf[2]&0x60)>>5;
			track_path=(pfi_buf[2]&0x10)>>4;
			printf("Disk has %d layers, ", layers+1);
			if(track_path==0)
				printf("Parallel Track Path\n");
			else
				printf("Opposite Track Path\n");
				
			// Now it's time to save the PFI
			if(layers>=1 && track_path == 0)
			{
				for(int i = 0; i<=layers; i++)
				{
					verified_read_PFI_layer(p_cdio, data_buffer_size, i);
				}
			}
			else
			{
				printf("Saving lead-in PFI to PFI.BIN\n");
            	file_ptr=fopen("PFI.BIN", "wb");
            	fwrite(pfi_buf, alloc_len, 1, file_ptr);
            	fclose(file_ptr);
            }
            
            has_bca=(pfi_buf[16]&0x80)>>7;
            
            if(has_bca==1)
            {
            	printf("Disk has Burst Cutting Area\n");
            	verified_read_BCA(p_cdio, data_buffer_size);
            }
            else
            	printf("Disk does not have Burst Cutting Area\n");
        }
        else
        {
            if(success==DRIVER_OP_MMC_SENSE_DATA)
            {
                disp_sense(p_cdio);
            }
            else
            {
                printf("Command sent but returned with an unhandled status code: %02X\n", success);
            }
        }
    }
    else
    {
        printf("failed.\n");
    }

    return success;
}

// Reads and dumps DVD Copyright Management Information
driver_return_code_t verified_read_CMI(CdIo_t *p_cdio,
                       unsigned long int data_buffer_size)
{
    driver_return_code_t success;
    unsigned short int alloc_len=0;
    unsigned char *cmi_buf;
   	FILE *file_ptr;

    printf("Sending read CMI command..");
    success=read_CMI(p_cdio, 65535, 0);
   	
    if(success==DRIVER_OP_SUCCESS)
    {
        alloc_len=data_buf[0] << 8;
        alloc_len=alloc_len | data_buf[1];
    }
    else
    {
        printf("done.\n");
        if(success==DRIVER_OP_MMC_SENSE_DATA)
        {
            disp_sense(p_cdio);
        }
        else
        {
            printf("Command sent but returned with an unhandled status code: %02X\n", success);
        }
    }

    if(success==DRIVER_OP_SUCCESS && (alloc_len>0))
    {
        //Limit alloc len to maximum allowed by size of data transfer buffer length.
        if((alloc_len+2)>data_buffer_size)
        {
            alloc_len=data_buffer_size-2;
        }

        success=read_CMI(p_cdio, alloc_len+4, 0);
        printf("done.\n");
        if(success==DRIVER_OP_SUCCESS)
        {
            alloc_len=data_buf[0] << 8;
            alloc_len=alloc_len | data_buf[1];
            alloc_len-=2;
            
            cmi_buf=data_buf+4;
        	printf("Saving Copyright Management Information as CMI.BIN\n");
       		file_ptr=fopen("CMI.BIN", "wb");
       		fwrite(cmi_buf, alloc_len, 1, file_ptr);
       		fclose(file_ptr);
        }
        else
        {
            if(success==DRIVER_OP_MMC_SENSE_DATA)
            {
                disp_sense(p_cdio);
            }
            else
            {
                printf("Command sent but returned with an unhandled status code: %02X\n", success);
            }
        }
    }
    else
    {
        printf("failed.\n");
        printf("Could only return 4 byte Read TOC header.\n");
    }

    return success;
}

// Reads DVD Disc Manufacturing Information
driver_return_code_t verified_read_DMI(CdIo_t *p_cdio,
                       unsigned long int data_buffer_size)
{
    driver_return_code_t success;
    unsigned short int alloc_len=0;
    unsigned char *dmi_buf;
   	FILE *file_ptr;

    printf("Sending read DMI command..");
    success=read_DMI(p_cdio, 65535, 0);
   	
    if(success==DRIVER_OP_SUCCESS)
    {
        alloc_len=data_buf[0] << 8;
        alloc_len=alloc_len | data_buf[1];
    }
    else
    {
        printf("done.\n");
        if(success==DRIVER_OP_MMC_SENSE_DATA)
        {
            disp_sense(p_cdio);
        }
        else
        {
            printf("Command sent but returned with an unhandled status code: %02X\n", success);
        }
    }

    if(success==DRIVER_OP_SUCCESS && (alloc_len>0))
    {
        //Limit alloc len to maximum allowed by size of data transfer buffer length.
        if((alloc_len+2)>data_buffer_size)
        {
            alloc_len=data_buffer_size-2;
        }

        success=read_DMI(p_cdio, alloc_len+4, 0);
        printf("done.\n");
        if(success==DRIVER_OP_SUCCESS)
        {
            alloc_len=data_buf[0] << 8;
            alloc_len=alloc_len | data_buf[1];
            alloc_len-=2;
            
            dmi_buf=data_buf+4;
        	printf("Saving Disc Manufacturer Information as DMI.BIN\n");
       		file_ptr=fopen("DMI.BIN", "wb");
       		fwrite(dmi_buf, alloc_len, 1, file_ptr);
       		fclose(file_ptr);
        }
        else
        {
            if(success==DRIVER_OP_MMC_SENSE_DATA)
            {
                disp_sense(p_cdio);
            }
            else
            {
                printf("Command sent but returned with an unhandled status code: %02X\n", success);
            }
        }
    }
    else
    {
        printf("failed.\n");
    }

    return success;
}

/* Sends Test Unit Ready command 3 times, check for errors & display error info. */
driver_return_code_t verified_test_unit_ready3(CdIo_t *p_cdio)
{
    unsigned char i=3;
    driver_return_code_t success;

    /*
    Before sending the required command, here we clear any pending sense info from the drive
    which may interfere by sending Test Unit Ready command at least 3 times if neccessary.
    ----------------------------------------------------------------------------------------*/
    do
    {
        printf("Sending SPC1 Test Unit CDB6 command..");
        //Sends SPC1 Test Unit Ready command to drive
        success=test_unit_ready(p_cdio);
        printf("done.\n");
        if(success==DRIVER_OP_SUCCESS)
        {
            printf("Returned good status.\n");
            i=1;
        }
        else
        {
            if(success==DRIVER_OP_MMC_SENSE_DATA)
            {
                disp_sense(p_cdio);
            }
            else
            {
                printf("Command sent but returned with an unhandled status code: %02X\n", success);
            }
        }
        i--;
    }while(i>0);

    return success;
}

// Find layer break from PFI and get disc full size
bool find_layers_size_from_PFI(CdIo_t *p_cdio,
						   unsigned char *pfi_buf,
                           unsigned char &out_n_tracks,
                           unsigned long int &out_n_sectors,
                           unsigned long int &layer_break)
{
    unsigned long int start_sector;
    unsigned long int end_sector;
    unsigned long int sector_len;

	out_n_tracks = 1; // DVD-ROMs only have 1 track

	start_sector=pfi_buf[5];
    start_sector=start_sector<<8;
    start_sector=start_sector | pfi_buf[6];
    start_sector=start_sector<<8;
    start_sector=start_sector | pfi_buf[7];

	end_sector=pfi_buf[9];
    end_sector=end_sector<<8;
    end_sector=end_sector | pfi_buf[10];
    end_sector=end_sector<<8;
    end_sector=end_sector | pfi_buf[11];
    
    if(start_sector != 0x30000 && start_sector != 0x31000)
    {
    	fprintf(stderr, "Unexpected start physical sector %lu\n", start_sector);
    	return false;
    }
    
    if(end_sector <= start_sector)
    {
    	fprintf(stderr, "End physical sector (%lu) is less than or equal to start physical sector (%lu).\n", end_sector, start_sector);
    	return false;
    }
    
    layer_break=pfi_buf[13];
    layer_break=layer_break<<8;
    layer_break=layer_break | pfi_buf[14];
    layer_break=layer_break<<8;
    layer_break=layer_break | pfi_buf[15];
    
    layer_break-=start_sector;
    layer_break++;
    
    read_CAPACITY(p_cdio);
    
    sector_len=data_buf[4];
    sector_len=sector_len<<8;
    sector_len=sector_len | data_buf[5];
    sector_len=sector_len<<8;
    sector_len=sector_len | data_buf[6];
    sector_len=sector_len<<8;
    sector_len=sector_len | data_buf[7];
    
    if(sector_len != 2048)
    {
    	fprintf(stderr, "Sector size should be 2048 bytes not %lu bytes", sector_len);
    	return false;
    }
    
    out_n_sectors=data_buf[0];
    out_n_sectors=out_n_sectors<<8;
    out_n_sectors=out_n_sectors | data_buf[1];
    out_n_sectors=out_n_sectors<<8;
    out_n_sectors=out_n_sectors | data_buf[2];
    out_n_sectors=out_n_sectors<<8;
    out_n_sectors=out_n_sectors | data_buf[3];
    out_n_sectors++;

    return true;
}

/*
Main loop of reading the DVD and writing to image file.
Various error checking are also done here.
*/
bool read_dvd_to_image(char *drive_letter, char *file_pathname, unsigned long int data_buffer_size)
{
    CdIo_t *p_cdio;
    bool cmd_ret;
    driver_return_code_t success;
    unsigned char n_tracks;  //Total tracks.
    unsigned long int n_sectors;  //Total sectors on CD.
    unsigned long int layer_break; // Layer break
    FILE *file_ptr;
    unsigned long int LBA_i;  //For counting "from" LBA (starts from 0).
    unsigned long int LBA_i2;  //For calculating "to" LBA.
    unsigned long int n_sectors_to_read;  //No. of sectors to read per read command.
   	unsigned char *pfi_buf;
   	unsigned char *tmp_buf;
   	
    p_cdio = open_volume(drive_letter);
    if (p_cdio != NULL)
    {
        printf("\n");
        success=verified_test_unit_ready3(p_cdio);
        printf("\n");
        if(success==DRIVER_OP_SUCCESS)
        {
            //Get TOC from CD.
            success=verified_read_PFI(p_cdio, data_buffer_size);
            if(success==DRIVER_OP_SUCCESS)
            {
            	pfi_buf=(unsigned char *)malloc(2048);
            	memset(pfi_buf, 0, 2048);
            	tmp_buf = data_buf+4;
            	memcpy(pfi_buf, tmp_buf, 2048);
            	
   				success=verified_read_CMI(p_cdio, data_buffer_size);
           	
            	success=verified_read_DMI(p_cdio, data_buffer_size);
            	
                if(find_layers_size_from_PFI(p_cdio, pfi_buf, n_tracks, n_sectors, layer_break))
                {
                    printf("Total user tracks : %u\n", n_tracks);
                    printf("Total sectors     : %lu\n", n_sectors);
                    printf("Layer break       : %lu\n", layer_break);
					
                        file_ptr=fopen(file_pathname, "wb");
                        if(file_ptr!=NULL)
                        {
                            n_sectors_to_read=data_buffer_size / 2048;  //Block size: E.g.: 65536 / 2048 = 32.
                            LBA_i=0;  //Starting LBA address.
                            while(LBA_i<n_sectors)
                            {
                                //Check if block size is suitable for the remaining sectors.
                                if(n_sectors_to_read>(n_sectors-LBA_i))
                                {
                                    //Alter to the remaining sectors.
                                    n_sectors_to_read=n_sectors-LBA_i;
                                }

                                LBA_i2=LBA_i+n_sectors_to_read-1;
                                printf("Reading sector %lu to %lu (total: %lu, progress: %.1f%%)\n", LBA_i, LBA_i2, n_sectors, (double)LBA_i2/n_sectors*100);
                                if(read_12(p_cdio, LBA_i, n_sectors_to_read)==DRIVER_OP_SUCCESS)
                                {
                                    if(success==DRIVER_OP_SUCCESS)
                                    {
                                        fwrite(data_buf, 2048*n_sectors_to_read, 1, file_ptr);
                                        if(ferror(file_ptr))
                                        {
                                            printf("Write file error!\n");
                                            printf("Aborting process.\n");
                                            cmd_ret=false;
                                            break;  //Stop while loop.
                                        }
                                    }
                                    else
                                    {
                                        if(success==DRIVER_OP_MMC_SENSE_DATA)
                                        {
                                            disp_sense(p_cdio);
                                        }
                                        else
                                        {
                                            printf("Command sent but returned with an unhandled status code: %02X\n", success);
                                        }

                                        printf("Aborting process.\n");
                                        break;  //Stop while loop.
                                    } 
                                }
                                else
                                {
                                    if(success==DRIVER_OP_MMC_SENSE_DATA)
                                    {
                                        disp_sense(p_cdio);
                                    }
                                    else
                                    {
                                        printf("Command sent but returned with an unhandled status code: %02X\n", success);
                                    }

                                    printf("Aborting process.\n");
                                    break;  //Stop while loop.
                                } 

                                LBA_i=LBA_i+n_sectors_to_read;
                            }

                            fclose(file_ptr);
                            cmd_ret=true;
                        }
                        else
                        {
                            printf("Could not create file!\n");
                            printf("Aborting process.\n");
                            cmd_ret = false;
                        }
                }
                else
                {
                    printf("Could not find layer break in PFI or get DVD size!\n");
                    printf("Aborting process.\n");
                    cmd_ret = false;
                }
            }
            else
            {
                printf("Could not read PFI!\n");
                printf("Aborting process.\n");
                cmd_ret = false;
            }
        }
        else
        {
            printf("Drive is not ready!\n");
            printf("Aborting process.\n");
            cmd_ret = false;
        }

        cdio_destroy(p_cdio);
    }
    else
    {
        return false;
    }

    return cmd_ret;
}

void usage()
{
    printf("DVDtoIMG v1.0. 21 Oct 2013.\n");
    printf("Usage: cdtoimg <drive path> <output file>\n");
    return ;  //Exit program here.
}

int main(int argc, char *argv[])
{
    if(argc == 3)
    {
        //65536 data transfer buffer.
        data_buf=(unsigned char *)malloc(65536);

        if(read_dvd_to_image(argv[1], argv[2], 65536))
        {
            printf("Process finished.");
        }
        else
        {
            printf("Could not create image from drive %c.", argv[1][0]);
        }

        free(data_buf);
        
        return 0;
    }
    else
    {
        usage();
        return -1;  //Exit program here.
    }
}