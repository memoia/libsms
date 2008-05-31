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
#include "sms.h"

/* 
 * approximate a magnitude spectrum by first downsampling using local maxima
 * and then upsampling using linear interpolation
 * the output spectrum doesn't have to be the same size as the input one
 *
 * float *pFSpec1     magnitude spectrum to approximate
 * int sizeSec1       size of input spectrum
 * int sizeSpec1Used  size of the spectrum to use
 * float *pFSpec2     output envelope
 * int sizeSpec2      size of output envelope
 * int nCoefficients  number of coefficients to use in approximation
 */
int SpectralApprox (float *pFSpec1, int sizeSpec1, int sizeSpec1Used,
                    float *pFSpec2, int sizeSpec2, int nCoefficients)
{
	float fHopSize, fCurrentLoc = 0, fLeft = 0, fRight = 0, fValue = 0, 
		fLastLocation, fSizeX, fSpec2Acum=0, fNextHop, fDeltaY, *pFEnvelope;
	int iFirstSample = 0, iLastSample = 0, i, j;

  	/* when number of coefficients is smaller than 2 do not approximate */
	if (nCoefficients < 2)
	{
		for (i = 0; i < sizeSpec2; i++)
			pFSpec2[i] = 1;
		return 1;
	}

	if ((pFEnvelope = (float *) calloc(nCoefficients, sizeof(float))) == NULL)
		return -1;
 
	/* calculate the hop size */
	if (sizeSpec1 != sizeSpec1Used)
		fHopSize = (float) sizeSpec1Used / nCoefficients;
	else //why is this here, would be the same as sizeSpec1Used / nCoefficients
		fHopSize = (float) sizeSpec1 / nCoefficients;
	
	/* approximate by linear interpolation */
	if (fHopSize > 1)
	{
		iFirstSample = 0;
		for (i = 0; i < nCoefficients; i++)
		{
			iLastSample = fLastLocation = fCurrentLoc + fHopSize;
			iLastSample = MIN (sizeSpec1-1, iLastSample);
			if (iLastSample < sizeSpec1-1)
				fRight = pFSpec1[iLastSample] +
					(pFSpec1[iLastSample+1] - pFSpec1[iLastSample]) * 
					(fLastLocation - iLastSample);
			else
				fRight = pFSpec1[iLastSample];
			fValue = 0;
			for (j = iFirstSample; j <= iLastSample; j++)
				fValue = MAX (fValue, pFSpec1[j]);
			fValue = MAX (fValue, MAX (fRight, fLeft));
			pFEnvelope[i] = fValue;
			fLeft = fRight;
			fCurrentLoc = fLastLocation;
			iFirstSample = (int) (1+ fCurrentLoc);
		}
	}
	else if (fHopSize == 1)
	{
		for (i = 0; i < nCoefficients; i++)
			pFEnvelope[i] = pFSpec1[i];
	}
	else
	{
		free (pFEnvelope);

		printf ("SpectralApprox: sizeSpec1 has too many nCoefficients\n");

		return -1;
	}

	/* Creates Spec2 from Envelope */
	if (nCoefficients < sizeSpec2)
	{
		fSizeX = (float) (sizeSpec2-1) / nCoefficients;

		/* the first step */
		fNextHop = fSizeX / 2;
		fDeltaY = pFEnvelope[0] / fNextHop;
		fSpec2Acum=pFSpec2[j=0]=0;
		while (++j < fNextHop)  
			pFSpec2[j] = (fSpec2Acum += fDeltaY);
     
		/* middle values */
		for (i = 0; i <= nCoefficients-2; ++i) 
		{
			fDeltaY = (pFEnvelope[i+1] - pFEnvelope[i]) / fSizeX;
			/* first point of a segment */
			pFSpec2[j] = (fSpec2Acum = (pFEnvelope[i]+(fDeltaY*(j-fNextHop))));
			++j;
			/* remaining points */
			fNextHop += fSizeX;
			while (j < fNextHop)  
				pFSpec2[j++] = (fSpec2Acum += fDeltaY);
    	}

		/* last step */
		fDeltaY = -pFEnvelope[i] * 2 / fSizeX;
		/* first point of the last segment */
		pFSpec2[j] = (fSpec2Acum = (pFEnvelope[i]+(fDeltaY*(j-fNextHop))));
		++j;
		fNextHop += fSizeX / 2;
		while (j < sizeSpec2-1)  
			pFSpec2[j++]=(fSpec2Acum += fDeltaY);
		/* last should be exactly zero */
		pFSpec2[sizeSpec2-1] = .0;	
    }
	else if (nCoefficients == sizeSpec2)
	{
		for (i = 0; i < nCoefficients; i++)
			pFSpec2[i] = pFEnvelope[i];
	}
	else
	{
		free (pFEnvelope);
		printf ("SpectralApprox: sizeSpec2 has too many nCoefficients\n");
		return -1;
	}
	free (pFEnvelope);
	return 1;
}
