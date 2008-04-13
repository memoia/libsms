/* 
 * Copyright (c) 2008 MUSIC TECHNOLOGY GROUP (MTG)
 *                         UNIVERSITAT POMPEU FABRA 
 * 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */
#include "../sms.h"

/* program written by Celso Mendonca de Aguiar */

#define USAGE "Usage: smsToClm <smsFile> <outputFile>"

short MaxDelayFrames;
float FResidualPerc;
SOUND_BUFFER soundBuffer, synthBuffer;
ANAL_FRAME **ppFrames, *pFrames;

void main (int ac, char *av[])
{
	SMSHeader *pSmsHeader;
	char *namedata, *outfile;
	SMS_DATA smsData;
	FILE *pSmsFile;
	int iError, havePhs, haveStoch, i, j, Ntrajet, NRec, counter;
	float amps, freqs, printfreqs, freqsm1, freqsp1, phs, gain, *freq_aver, Sum, min = 22050.0;
	FILE *fp;
  
	if (ac != 3) quit(USAGE);
	outfile = av[2];
	fp = fopen(outfile,"w");
	namedata = (char *)strtok (outfile,".");
	if((iError = GetSmsHeader (av[1], &pSmsHeader, &pSmsFile)) < 0)
	{ 
		if (iError == SMS_NOPEN)  printf("cannot open file");
		if (iError == SMS_RDERR)  printf("read error");
		if (iError == SMS_NSMS)   printf("not an SMS file");
		if (iError == SMS_MALLOC) printf("cannot allocate memory");
		printf("error");
	}
	AllocSmsRecord (pSmsHeader, &smsData);

	havePhs = (!(pSmsHeader->iFormat == FORMAT_HARMONIC ||
               pSmsHeader->iFormat == FORMAT_INHARMONIC));
	haveStoch = (pSmsHeader->iStochasticType != STOC_NONE);
	Ntrajet = pSmsHeader->nTrajectories;
	NRec = pSmsHeader->nRecords;

	fprintf(fp,"#|\n");
	fprintf(fp, "This lisp was generated by smsToClm. \n\n");
	fprintf(fp, "HEADER INFORMATION:\n");
	fprintf(fp, "Number of records = %d\n", pSmsHeader->nRecords);
	fprintf(fp, "Frame rate = %d\n", pSmsHeader->iFrameRate);
	fprintf(fp, "Number of trajectories = %d\n", pSmsHeader->nTrajectories);
	fprintf(fp, "Number of coefficients = %d\n", pSmsHeader->nStochasticCoeff);
	fprintf(fp, "format = %d\n", pSmsHeader->iFormat);
	fprintf(fp, "stochastic type = %d\n", pSmsHeader->iStochasticType);
	fprintf(fp, "sampling rate = %d\n", pSmsHeader->iOriginalSRate);  
      
	if (pSmsHeader->nTextCharacters > 0) 
	{
		fprintf(fp, "\nANALYSIS ARGUMENTS:\n");
		fprintf(fp, "%s\n", pSmsHeader->pChTextCharacters);
	} 
	fprintf(fp,"|#\n\n");

	fprintf(fp, "(defparameter amp-%s (make-array %i :element-type 'array))\n", namedata, Ntrajet);
	fprintf(fp, "(defparameter frq-%s (make-array %i :element-type 'array))\n", namedata, Ntrajet);
	if (havePhs)
		fprintf(fp, "(defparameter pha-%s (make-array %i :element-type 'array))\n", namedata, 
		        Ntrajet);
	if (haveStoch)
	       fprintf(fp, "(defparameter coef-%s (make-array %i :element-type 'array))\n", namedata, NRec);
	fprintf(fp,"\n");

	for(j = 0; j < Ntrajet; j++) 
	{
		fprintf (fp,"(setf (aref amp-%s %i) (make-array %i :initial-contents '(", namedata, j, NRec);
		for(i = 0; i < NRec; i++) 
		{
			GetSmsRecord (pSmsFile, pSmsHeader, i, &smsData);
			amps = TO_MAG (smsData.pFMagTraj[j]);
			fprintf (fp,"%f ", amps);
		}
		fprintf (fp,")))\n\n");
	}

	freq_aver = (float *) malloc(Ntrajet * sizeof(float));
	for(j = 0; j < Ntrajet; j++) 
	{
		fprintf(fp,"(setf (aref frq-%s %i) (make-array %i :initial-contents '(", namedata, j, NRec);
		counter = 0;
		Sum = 0;
		freqsm1 = 0.0;
		GetSmsRecord(pSmsFile, pSmsHeader, i, &smsData);
		freqs = smsData.pFFreqTraj[j];
		for(i = 0; i < NRec; i++) 
		{
			GetSmsRecord(pSmsFile, pSmsHeader, i, &smsData);
			freqsp1 = smsData.pFFreqTraj[j];
			printfreqs = freqs;
			if (freqs<0.000001 && freqsm1>0.0) printfreqs = freqsm1;
			if (freqs<0.000001 && freqsp1>0.0) printfreqs = freqsp1;
			Sum = Sum + freqs;
			if (freqs>0) counter++;
			fprintf(fp,"%f ", printfreqs);
			freqsm1 = freqs;
			freqs = freqsp1;			
		}
		freq_aver[j] = (float) (Sum / counter);
		fprintf(fp,")))\n\n");
	}

	fprintf (fp,"(defparameter freq-%s '(", namedata);
	for (j = 0; j < Ntrajet; j++) 
	{
		if (freq_aver[j] < min) min = freq_aver[j];
		fprintf (fp,"%f ", freq_aver[j]); 
	}
	fprintf (fp,"))\n\n(defparameter fund-%s %f)\n\n", namedata, min);

	if (havePhs) 
	{
		for (j = 0; j < Ntrajet; j++) 
		{
			fprintf (fp,"(setf (aref pha-%s %i) (make-array %i :initial-contents '(", namedata, j, NRec);
			for (i = 0; i < NRec; i++) 
			{
				GetSmsRecord (pSmsFile, pSmsHeader, i, &smsData);
				phs = smsData.pFPhaTraj[j];
				fprintf (fp,"%f ", phs);
			}
			fprintf (fp,")))\n\n");
		}
	}

	if (haveStoch) 
	{		   
		fprintf (fp,"(defparameter gain-%s (make-array %i :initial-contents '(", namedata, NRec);
		for (i = 0; i < NRec; i++)
		{
			GetSmsRecord (pSmsFile, pSmsHeader, i, &smsData);
			gain = TO_MAG (*(smsData.pFStocGain));
			fprintf (fp,"%f ", gain);
		}
		fprintf (fp,")))\n\n");

		for(i = 0; i < NRec; i++) 
		{
			GetSmsRecord (pSmsFile, pSmsHeader, i, &smsData);
			fprintf (fp,"(setf (aref coef-%s %i) (make-array %i :initial-contents '(0.00 ", namedata, i, 
			              2+pSmsHeader->nStochasticCoeff);
		        for (j = 0; j < smsData.nCoeff; j++)
			{
				fprintf (fp,"%f ", smsData.pFStocCoeff[j]);
			}
			fprintf (fp,"0.00)))\n\n");
		}
	}
	free (pSmsHeader);
	fclose (pSmsFile);
	exit(0);
}