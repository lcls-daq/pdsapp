TCanvas* createCanvas(const char* title,
		      int nx,
		      int ny,
		      int mode)
{
  TCanvas* c0;
  switch(mode%4) {
  case 0: // landscape
    c0 = new TCanvas(title, title,395,5,800,600);
    break;
  case 1: // portrait
    c0 = new TCanvas(title, title,395,5,600,800);
    break;
  case 2: // square
    c0 = new TCanvas(title, title,395,5,600,600);
    break;
  case 3: // half-square
  default:
    c0 = new TCanvas(title, title,395,5,600,300);
    break;
  }    
  c0->cd();
  TLatex* text = new TLatex(0.4,0.98,title);
  text->SetTextSize(0.02);
  text->Draw();
  if (mode<4)
    c0->Divide(nx,ny,0.01,0.02);
  else
    c0->Divide(nx,ny,0.001,0.001);
  return c0;
}

void setColorMap() {

  const int nshades = 8;
  const int numcol = 5*nshades;
  static float shades[] = { 0.00, 0.4, 0.5, 0.6,
			    0.7, 0.8, 0.9, 1.0 };

  /*
  const int nshades = 5;
  const int numcol = 5*nshades;
  static float shades[] = { 0.00, 0.4, 0.6,
			    0.8, 1.0 };
  */
  Int_t* colors = new int[numcol];

  float r=1.,g=shades[0],b=shades[0];
  float dc = 1./float(nshades);
  for(unsigned j=0; j<numcol; j++) {
    if (j<nshades)
      g = shades[j];
    else if (j<2*nshades)
      r = shades[2*nshades-j-1];
    else if (j<3*nshades) 
      b = shades[j-2*nshades];
    else if (j<4*nshades)
      g = shades[4*nshades-j-1];
    else
      r = shades[j-4*nshades];
    int icol = j+20;
    if (!gROOT->GetColor(icol)) {
      char name[20];
      sprintf(name,"color%d",icol);
      TColor* color = new TColor(icol,r,g,b,name);
    }
    else {
      TColor* color = gROOT->GetColor(icol);
      color->SetRGB(r,g,b);
    }
    colors[j] = icol;
  }
  gStyle->SetPalette(numcol,colors);
}

TH1D* make_hist(const char* ttl, float* d, unsigned n, int ci) 
{
  TH1D* h = new TH1D(ttl,ttl,200,0.,200.);

  const unsigned len = 10000;
  for(unsigned i=0; i<len-n; i++) {
    double dt = 0;
    for(unsigned j=0; j<n; j++)
      dt += d[i+j];
    h->Fill(dt*1.e3/double(n));
  }
  h->SetLineColor(ci);
  h->SetLineWidth(2);
  return h;
}

void make_pad(const char* fname)
{
  FILE* f = fopen(fname,"r");
  if (!f) return;

  const unsigned len = 10000;
  float* dt = new float[len];
  for(unsigned i=0; i<len; i++)
    fscanf(f,"%g",&dt[i]);
  fclose(f);

  TH1D* h[7];
  h[0] = make_hist("h_1"  ,dt, 1  , 1);
  h[1] = make_hist("h_16" ,dt, 16 ,21);
  h[2] = make_hist("h_32" ,dt, 32 ,26);
  h[3] = make_hist("h_64" ,dt, 64 ,31);
  h[4] = make_hist("h_128",dt, 128,36);
  h[5] = make_hist("h_256",dt, 256,41);
  h[6] = make_hist("h_512",dt, 512,46);

  h[0]->Draw();
  h[0]->SetMaximum(h[6]->GetMaximum());
  h[0]->GetXaxis()->SetTitle("Disk Write Times [ms]");

  double x = 140;
  double y = h[0]->GetMaximum();
  double ry = pow(1./y,0.1);
  for(unsigned i=0; i<7; i++) {
    if (i==0)
      h[i]->Draw();
    else
      h[i]->Draw("same");
    y *= ry;
    TText* t = new TText(x,y,h[i]->GetTitle());
    t->SetTextColor(h[i]->GetLineColor());
    t->Draw();
  }
}

void make_canvas(const char* title,
                 const char* path)
{
  TCanvas* c = createCanvas(title,1,1,2);
  c->cd(1);
  c->Pad()->SetLogy();
  make_pad(path);
}

void dsstest1(const char* path)
{
  setColorMap();
  gStyle->SetOptStat(0);
  make_canvas("DSS",path);
}

