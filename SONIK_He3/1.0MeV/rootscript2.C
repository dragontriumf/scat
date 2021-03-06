#include <iostream>

#include <TObject.h>
#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TLeaf.h>
#include <TH1F.h>
#include <TH1.h>
#include <TH3F.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TString.h>

#include <TROOT.h>

#include "/local/astro/scat/src/CrossCalc.hxx"

Double_t Get(TString name, TBranch* b, Float_t min, Float_t max, TString req, Int_t bins)
{
	TObject* old=gDirectory->GetList()->FindObject(name);
	gDirectory->GetList()->Remove(old);
	static Float_t leaf;
	static Float_t old_leaf;
	Int_t nevents = (Int_t)b->GetEntries();
	TLeaf *l = b->GetLeaf(name);
	l->SetAddress(&leaf);
	TH1F *h = new TH1F(name,name,bins,min,max);
	for (Int_t j=0;j<nevents;j++)
	{
		b->GetEntry(j);
		h->Fill(leaf);
	}
	l->SetAddress(&old_leaf);
	if (req == TString("mean")) {return (Double_t)h->GetMean();}
	else if (req == TString("rms")) {return (Double_t)h->GetRMS();}
	else if (req == TString("fwhm"))
	{
		Int_t bin1 = h->FindFirstBinAbove(h->GetMaximum()/2);
   		Int_t bin2 = h->FindLastBinAbove(h->GetMaximum()/2);
   		Double_t fwhm = h->GetBinCenter(bin2) - h->GetBinCenter(bin1);
		return fwhm;
	}
	else {return (Double_t) -1.;}
}

TH1F *GetHist(TString name, TBranch* b, Int_t K, Int_t bins)
{
	const Float_t ThetaCM[11] = {39.265, 43.57, 60.61, 68.97, 77.2, 93.12, 100.74, 108.07, 121.71, 138.9, 160.74};
	TObject* old=gDirectory->GetList()->FindObject(name);
	gDirectory->GetList()->Remove(old);
	static Float_t leaf;
	static Float_t old_leaf;
	Int_t nevents = (Int_t)b->GetEntries();
	TLeaf *l = b->GetLeaf(name);
	l->SetAddress(&leaf);
	TH1F *h = new TH1F(name,name,bins,ThetaCM[K]-5.,ThetaCM[K]+5.);
	for (Int_t j=0;j<nevents;j++)
	{
		b->GetEntry(j);
		h->Fill(leaf);
	}
	l->SetAddress(&old_leaf);
	return h;
}

void rootscript2()
{
	TFile *f;
	TTree *t;
	TBranch *b;

	TGraphErrors *gr[5];	
	Double_t x[11]; //= {22.5, 25., 35., 40., 45., 55., 60., 65., 75., 90., 120.};
	Double_t ex[11] = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., };
	Double_t y[11];
	Double_t ey[11] = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., };

	const TString col[] = {"1", "2", "3", "5", "7"};
	const Double_t colr[] = {0.5, 1.0, 1.5, 2.5, 3.5};//radius of colimator sizes
	//const Double_t ra[] = {0.35, 0.356, 0.391, 0.415, 0.445, 0.529, 0.589, 0.664, 0.884, 1.521, 5.221}; //cm/lab ratio 
	const TString deg[] = {"22.5", "25", "35", "40", "45", "55", "60", "65", "75", "90", "120"};
	const TString sls = "mm/";
	const TString evn = "deg/Events0.root";

	const Double_t Zr[5][11] = {{0.8181, 0.7410, 0.5459, 0.4870, 0.4424, 0.3818, 0.3616, 0.3452, 0.3240, 0.3128, 0.3616},{0.8178, 0.7411, 0.5458, 0.4871, 0.4422, 0.3819, 0.3615, 0.3451, 0.3240, 0.3130, 0.3615}, {0.817, 0.742, 0.547, 0.488, 0.443, 0.383, 0.362, 0.346, 0.324, 0.315, 0.362}, {0.814, 0.740, 0.547, 0.488, 0.442, 0.383, 0.362, 0.346, 0.325, 0.315, 0.362}, {0.807, 0.738, 0.547, 0.488, 0.443, 0.383, 0.362, 0.346, 0.324, 0.314, 0.362}}; //effective z from detView.C
	
	const Double_t Theta[11] = {22.5, 25., 35., 40., 45., 55., 60., 65., 75., 90., 120.};
	const Double_t ThetaCM[11] = {39.265, 43.57, 60.61, 68.97, 77.2, 93.12, 100.74, 108.07, 121.71, 138.9, 160.74};

	const Double_t P = 4.98; //pressure in torr
  	const Double_t avo = 6.022e+23; // molecules per mole
  	const Double_t temp = 21.0; // Central temperature (Centigrade)
  	const Double_t R = 0.08206; // Gas constant (L.atm/mol.K)
	const Double_t pi = 3.14159265; //pi

	const Double_t pnA = 10000. * 6.25e9;
	const Double_t nVol = (Double_t) (P/760.0) * avo / R / (temp+273.15) / 1000.; // atoms/cm^3

	const Double_t m1 = 2809.413; //MeV
	const Double_t m2 = 3728.401; //MeV
	const Double_t E1 = m1 + 0.876; //m + T MeV

	static ResonanceCalculator rc("/local/astro/scat/3He(a,a)_abinitio.dat");

	for (int p=0;p<5;p++)
	{
		for (int q=0;q<11;q++)
		{
			TString str = "/local/astro/scat/SONIK_He3/1.0MeV/";
			str.Append(col[p]);
			str.Append(sls);
			/*
			if (p == 0) { str = "/local/astro/scat/SONIK_He3/0.5MeV/1mm_200k_pnA/";}
			if (p == 0 && q >= 9) { str = "/local/astro/scat/SONIK_He3/0.5MeV/1mm_1M_pnA/";}
			if (p == 1) { str = "/local/astro/scat/SONIK_He3/0.5MeV/2mm_300k_pnA/";}
			if (p == 1 && q == 10) { str = "/local/astro/scat/SONIK_He3/0.5MeV/2mm_1M_pnA/";}
			*/
			if (p == 0) {str = "/local/astro/scat/SONIK_He3/1.0MeV/1mm_10k_pnA/";}
			if (p == 1) {str = "/local/astro/scat/SONIK_He3/1.0MeV/2mm_10k_pnA/";}
			if (p == 2) {str = "/local/astro/scat/SONIK_He3/1.0MeV/3mm_10k_pnA/";}
			if (p == 3) {str = "/local/astro/scat/SONIK_He3/1.0MeV/5mm_10k_pnA/";}
			if (p == 4) {str = "/local/astro/scat/SONIK_He3/1.0MeV/7mm_10k_pnA/";}

			str.Append(deg[q]);
			str.Append(evn);

			f = new TFile(str);
			t = (TTree*)f->Get("Ejectile Detector Events");
			b = t->GetBranch("Events");
	
			x[q] = ThetaCM[q]; 
			Double_t tht = Theta[q] * 0.01745;
			Double_t thtcm = x[q] * 0.01745;
			Double_t nevents = (Double_t)b->GetEntries();
			/////////////////////////////////////////////////////////////////////////// SOLID ANGLE CALCULATION
			Double_t SolidAngle = pi * pow(colr[p]/170.,2); 
			/////////////////////////////////////////////////////////////////////////// Z RANGE CALCULATION
			Double_t z = Zr[p][q];
			/////////////////////////////////////////////////////////////////////////// CONVERSION TO CENTER OF MASS
			Double_t gamma = (E1 + m2) / sqrt(pow(m1,2) + pow(m2,2) + 2*m2*E1);
			Double_t deltacm = m1 / m2; //delta(2,3)=beta(2)/beta(3) in cm frame, reduces to just ratio of masses
			Double_t conv = pow(sin(tht)/sin(thtcm),3)*gamma*(deltacm*cos(thtcm) + 1.);	
			/////////////////////////////////////////////////////////////////////////// CROSS SECTION CALCULATION
			Double_t cx = (Double_t) conv * nevents/(SolidAngle*pnA*nVol*z); //cross section in cm^2/sr
			/*
			if (p == 0) {cx = cx / 20.;}
			if (p == 0 && q >= 9) { cx = cx / 5.;}
			if (p == 1) {cx = cx / 30.;}
			if (p == 1 && q == 10) { cx = cx * 30. / 100.;}
			*/
			/////////////////////////////////////////////////////////////////////////// CONVERSION TO MILLIBARNS PER STERADIANS
			y[q] =  cx * 1.0e27; //cross section in mb/sr
			/////////////////////////////////////////////////////////////////////////// ERROR OF CONVERSION
			//taken to be absolute deviation of the conversion factor at thetacm + (1/4)fwhm of thetacm
			//error of gamma factor is negligible here
			Double_t thtcm2 = thtcm + (0.25*Get("theta_cm",b,0,180,"fwhm",10000) * 0.01745); // 1/4 of fwhm
			Double_t tht2 = tht + (0.25*Get("theta_lab",b,0,180,"fwhm",10000) * 0.01745); 
			Double_t errconv = (Double_t) abs( pow(sin(tht2)/sin(thtcm2),3)*gamma*(deltacm*cos(thtcm2) + 1.) - conv );
			/////////////////////////////////////////////////////////////////////////// ERROR OF SOLID ANGLE
			//taken to be absolute deviation of the SA a distance (z range)/2 from the center z=20
			tht = Theta[q] * 0.01745;
			Double_t h = 172. * sin(tht);
			Double_t z0 = 172. * cos(tht);
			z0 = z0 - (z/2);
			Double_t d = sqrt(pow(z0,2)+pow(h,2));
			Double_t errSA = abs( pi * pow(colr[p]/d,2) - SolidAngle); 
			/////////////////////////////////////////////////////////////////////////// STATISICAL ERROR AND SUM OF ERRORS
			ex[q] = Get("theta_cm",b,0,180,"fwhm",10000) / 2.;
			ey[q] = y[q] * ( (sqrt(nevents)/nevents) );//+ (errconv/conv) + (errSA/SolidAngle) );
			/////////////////////////////////////////////////////////////////////////// DIVIDE CROSS SECTION BY ACTUAL
			//y[q] =  y[q] / rc.Calculate(Get("E_beam",b,0,3,"mean",10000), x[q]);
			//ey[q] = ey[q] / rc.Calculate(Get("E_beam",b,0,3,"mean",10000), x[q]);
			cout << z << "  " << SolidAngle << "  " << conv << "  " << y[q] << "  " << nevents << "  " << errSA/SolidAngle << "  " << errconv/conv << endl;
			f->Close("R");	
		}
		cout << endl;
		gr[p] = new TGraphErrors(11,x,y,ex,ey);
	}

	TGraph *inputcx;
	for (int q=0;q<11;q++)
	{
		x[q] = ThetaCM[q];
		y[q] = rc.Calculate(1.0,x[q]);
	}
	inputcx = new TGraph(11,x,y);

	TCanvas *c1 = new TCanvas("c1","Cross Section (mb/sr) vs. Theta CM", 1);
	c1->SetFillColor(10);
	c1->SetGrid();
	c1->SetLogy();
	
	gr[2]->SetLineWidth(2);
	gr[2]->SetLineColor(4);
	gr[2]->SetLineStyle(2);
	gr[2]->SetMarkerColor(4);
	gr[2]->SetMarkerStyle(21);
	gr[2]->Draw("ALP");
	gr[2]->SetTitle("Cross Section vs. #theta_{cm}");
	gr[2]->GetXaxis()->SetRangeUser(0.,180.);
	//gr[2]->GetYaxis()->SetRangeUser(0.,2.);
	gr[2]->GetXaxis()->SetTitle("#theta_{cm} (deg)");
	gr[2]->GetYaxis()->SetTitle("d#sigma/d#Omega (mb/sr)");
	gr[2]->GetXaxis()->CenterTitle();
	gr[2]->GetYaxis()->CenterTitle();
	for (int p=0;p<2;p++)
	{
		gr[p]->SetLineWidth(2);
		gr[p]->SetLineColor(6+p);
		gr[p]->SetLineStyle(2);
		gr[p]->SetMarkerColor(6+p);
		gr[p]->SetMarkerStyle(21);
		//gr[p]->SetTitle("p");
		gr[p]->Draw("LP");
	}	

	inputcx->SetLineWidth(2);
	inputcx->SetLineColor(2);
	inputcx->SetLineStyle(2);
	inputcx->SetMarkerColor(2);
	inputcx->SetMarkerStyle(21);
	inputcx->Draw("LP");

	TLegend *leg = new TLegend(0.4,0.6,0.89,0.89);	
	leg->AddEntry(gr[0],"1mm Collimator","pf");
	leg->AddEntry(gr[1],"2mm Collimator","pf");
	leg->AddEntry(gr[2],"3mm Collimator","pf");
	//leg->AddEntry(gr[3],"5mm Collimator","pf");
	//leg->AddEntry(gr[4],"7mm Collimator","pf");
	leg->AddEntry(inputcx,"Input Cross Section","pf");
	leg->Draw(); 	

	gr[0]->SetName("gr0");
	gr[1]->SetName("gr1");
	gr[2]->SetName("gr2");

	c1->Update();
	//c1->AddExec("data","DataExec()");

	TCanvas *c2 = new TCanvas("c2","Cross Section (mb/sr) vs. Theta CM", 1);
	c2->SetFillColor(10);
	c2->SetGrid();
	c2->SetLogy();	

	gr[4]->SetLineWidth(2);
	gr[4]->SetLineColor(4);
	gr[4]->SetLineStyle(2);
	gr[4]->SetMarkerColor(4);
	gr[4]->SetMarkerStyle(21);
	gr[4]->Draw("ALP");
	gr[4]->SetTitle("Cross Section vs. #theta_{cm}");
	gr[4]->GetXaxis()->SetRangeUser(0.,180.);
	//gr[4]->GetYaxis()->SetRangeUser(0.,2.);
	gr[4]->GetXaxis()->SetTitle("#theta_{cm} (deg)");
	gr[4]->GetYaxis()->SetTitle("d#sigma/d#Omega (mb/sr)");
	gr[4]->GetXaxis()->CenterTitle();
	gr[4]->GetYaxis()->CenterTitle();
	for (int p=3;p<4;p++)
	{
		gr[p]->SetLineWidth(2);
		gr[p]->SetLineColor(5+p);
		gr[p]->SetLineStyle(2);
		gr[p]->SetMarkerColor(5+p);
		gr[p]->SetMarkerStyle(21);
		//gr[p]->SetTitle("p");
		gr[p]->Draw("LP");
	}	

	inputcx->SetLineWidth(2);
	inputcx->SetLineColor(2);
	inputcx->SetLineStyle(2);
	inputcx->SetMarkerColor(2);
	inputcx->SetMarkerStyle(21);
	inputcx->Draw("LP");

	TLegend *leg2 = new TLegend(0.4,0.6,0.89,0.89);	
	//leg->AddEntry(gr[0],"1mm Collimator","pf");
	//leg->AddEntry(gr[1],"2mm Collimator","pf");
	//leg->AddEntry(gr[2],"3mm Collimator","pf");
	leg2->AddEntry(gr[3],"5mm Collimator","pf");
	leg2->AddEntry(gr[4],"7mm Collimator","pf");
	leg2->AddEntry(inputcx,"Input Cross Section","pf");
	leg2->Draw(); 

	c2->Update();	

	cout << endl;
}

void DataExec()
{
	gPad->GetCanvas()->FeedbackMode(kTRUE);

	TGraphErrors *gr[3];
	gr[0] = (TGraphErrors*)gPad->GetCanvas()->GetListOfPrimitives()->FindObject("gr0");
	gr[1] = (TGraphErrors*)gPad->GetCanvas()->GetListOfPrimitives()->FindObject("gr1");
	gr[2] = (TGraphErrors*)gPad->GetCanvas()->GetListOfPrimitives()->FindObject("gr2");

	double px = gPad->GetEventX();
	double py = gPad->GetEventY();
	px = gPad->AbsPixeltoX(px);
	py = gPad->AbsPixeltoY(py);
	px = gPad->PadtoX(px);
	py = gPad->PadtoY(py);

	double dmin=9999;
	int J, K;
	static int oldJ, oldK;
	for (int j=0; j<3; j++)
	{
		double *X;
		double *Y;
		X = gr[j]->GetX();
		Y = gr[j]->GetY();
		for (int k=0; k<11; k++)
		{
			if (sqrt(pow(px - *(X+k),2) + pow(py - *(Y+k),2)) < dmin)
			{
				dmin = sqrt(pow(px - *(X+k),2) + pow(py - *(Y+k),2));
				J = j;
				K = k;
			}
		}
	}

	if ( J != oldJ || K != oldK ) 
	{
		oldJ = J;
		oldK = K;

		const TString col[] = {"1", "2", "3", "5", "7"};
		const TString deg[] = {"22.5", "25", "35", "40", "45", "55", "60", "65", "75", "90", "120"};
		const TString sls = "mm/";
		const TString evn = "deg/Events0.root";

		TString str = "/local/astro/scat/SONIK_He3/0.5MeV/";
		str.Append(col[J]);
		str.Append(sls);
		/*
		if (J == 0) { str = "/local/astro/scat/SONIK_He3/0.5MeV/1mm_200k_pnA/";}
		if (J == 0 && K >= 9) { str = "/local/astro/scat/SONIK_He3/0.5MeV/1mm_1M_pnA/";}
		if (J == 1) { str = "/local/astro/scat/SONIK_He3/0.5MeV/2mm_300k_pnA/";}
		if (J == 1 && K == 10) { str = "/local/astro/scat/SONIK_He3/0.5MeV/2mm_1M_pnA/";}
		*/	
		if (J == 0) {str = "/local/astro/scat/SONIK_He3/0.5MeV/1mm_10k_pnA/";}
		if (J == 1) {str = "/local/astro/scat/SONIK_He3/0.5MeV/2mm_10k_pnA/";}
		if (J == 2) {str = "/local/astro/scat/SONIK_He3/0.5MeV/3mm_10k_pnA/";}
			
		str.Append(deg[K]);
		str.Append(evn);

		TFile *f = new TFile(str);
		TTree *t = (TTree*)f->Get("Ejectile Detector Events");
		TBranch *b = t->GetBranch("Events");
		Double_t nevents = (Double_t)b->GetEntries();
		/*
		if (J == 0) {nevents = nevents / 20.;}
		if (J == 0 && K >= 9) { nevents = nevents / 5.;}
		if (J == 1) {nevents = nevents / 30.;}
		if (J == 1 && K == 10) { nevents = nevents * 30. / 100.;}
		*/
		int pnA = 10000;
		/*
		int pnA = J == 0 ? K >= 9 ? 1000000 : 200000 :
			  J == 1 ? K == 10 ? 1000000 : 300000 :
			  10000;
		*/
		cout << "\033[F" << "\033[J" << "Collimator: " << col[J] << "mm  Angle: " << deg[K] << "degrees  Beam: " << pnA << "pnA  Detected Events(per 10000pnA): " << nevents << endl;
		//create or set the new canvas c2
		TVirtualPad *padsav = gPad;
		TCanvas *c3 = (TCanvas*)gROOT->FindObject("c3");
		if(c3) ;//delete c2->GetPrimitive("Projection");
		else   c3 = new TCanvas("c3","Theta CM Histogram",710,10,700,500);
		c3->SetGrid();
		c3->cd();
		GetHist("theta_cm",b,K,100)->Draw();
		c3->Update();
		padsav->cd();

		f->Close("R");
	}
}
	 	

