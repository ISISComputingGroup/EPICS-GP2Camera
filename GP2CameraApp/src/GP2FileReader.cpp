#include <iostream>
#include <stdio.h>
#include <io.h>
#include <epicsTime.h>

int main(int argc, char* argv[])
{
    epicsTimeStamp epicsTS;
    int imageCounter, numImagesCounter;
    size_t nelements;
    epicsInt16* value;
	char tbuffer[60];
//	epicsTimeToStrftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S.%06f", epicsTS);

    bool display = false;
    FILE* infile = fopen(argv[1], "rb");
	if (infile == NULL)
	{
	    std::cerr << "Cannot open file " << argv[1] << std::endl;
		return -1;
	}
	const char* opts = "";
	if (argc > 2)
	{
	    opts = argv[2];
	}
	display = (strchr(opts, 'd') != NULL);
	int frames = 0, nevents = 0;
    while(true)
	{
			if (fread(&epicsTS, sizeof(epicsTimeStamp), 1, infile) != 1)
			    break;
			epicsTimeToStrftime(tbuffer, sizeof(tbuffer), "%Y-%m-%dT%H:%M:%S.%06f", &epicsTS);
			if (fread(&imageCounter, sizeof(int), 1, infile) != 1)
			    break;
			if (fread(&numImagesCounter, sizeof(int), 1, infile) != 1)
			    break;
			if (fread(&nelements, sizeof(size_t), 1, infile) != 1)
			    break;
			value = new epicsInt16[nelements];
			if (fread(value, sizeof(epicsInt16), nelements, infile) != nelements)
			    break;
			++frames;
			nevents += nelements;
			if (display)
			{
			    for(int i=0; i<nelements; i += 3)
			    {
				    std::cout << tbuffer << "," << numImagesCounter;
				    for(int j=0; j<3; ++j)
				    {
				        std::cout << "," << value[i+j];	
				    }					
			        std::cout << std::endl;
				}
			}
			delete[] value;
	}
    fclose(infile);
	std::cerr << "Frames = " << frames << ", events = " << nevents << ", imagesCounter = " << numImagesCounter << std::endl;
	return 0;
}
