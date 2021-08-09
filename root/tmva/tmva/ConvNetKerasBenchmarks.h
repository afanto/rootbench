#include "TMVA/Factory.h"
#include "TMVA/DataLoader.h"
#include "TMVA/Config.h"
#include "TMVA/PyMethodBase.h"
#include "TMVA/DataSetInfo.h"
#include "TFile.h"
#include "TMacro.h"
#include "TSystem.h"
#include "TTree.h"
#include "MakeImageData.h"

void CNN_Keras_benchmark(TString archName) {

	int ntrainEvts = 500;
	int ntestEvts =  500;

	size_t nx = 32;
	size_t ny = 32;

	ROOT::EnableImplicitMT(0);
	TMVA::Config::Instance();

	// for using Keras
	gSystem->Setenv("KERAS_BACKEND", "tensorflow");
	TMVA::PyMethodBase::PyInitialize();

	// Load the input data
	TString fname = "imagesData.root";
	TString fopt = "CACHEREAD";

	// Make some Gaussian Images.
	makeImages(ntrainEvts + ntestEvts, nx, ny);

	auto input = TFile::Open(fname, fopt);

	R__ASSERT(input);

	std::cout << "--- Classification  : Using input file: " << input->GetName() << std::endl;

	// Create a ROOT output file where TMVA will store ntuples, histograms, etc.
	TString outfileName( "output.root" );
	TFile* outputFile = TFile::Open(outfileName, "RECREATE");

	// Creating the factory object
	TMVA::Factory *factory = new TMVA::Factory( "TMVAClassification", outputFile,
		                                       "!Correlations:!V:!Silent:Color:DrawProgressBar:"
		                                       "AnalysisType=Classification:!ModelPersistence:Transformations=None" );
	TMVA::DataLoader *dataloader = new TMVA::DataLoader("dataset_cnn_ecal");

	TTree *signalTree = (TTree*)input->Get("sgn");
	TTree *background = (TTree*)input->Get("bkg");

	// add variables (time zero and time 1)
	for (size_t j = 0; j < nx * ny; ++j) {
	  TString varName = TString::Format("var%zu", j);
	  dataloader->AddVariable(varName, 'F');
	}

	dataloader->AddSignalTree    ( signalTree, 1.0 );
	dataloader->AddBackgroundTree( background, 1.0 );

	// check given input
	auto & datainfo = dataloader->GetDataSetInfo();
	auto vars = datainfo.GetListOfVariables();
	std::cout << "number of variables is " << vars.size() << std::endl;

	TString trainAndTestOpt = TString::Format("nTrain_Signal=%d:nTrain_Background=%d:nTest_Signal=%d:nTest_Background=%d:"
		                                     "SplitMode=Random:NormMode=NumEvents:!V",
		                                     ntrainEvts, ntrainEvts, ntestEvts, ntestEvts);
	TCut mycuts = "";
	TCut mycutb = "";
	dataloader->PrepareTrainingAndTestTree(mycuts, mycutb, trainAndTestOpt);

	TMVA::MsgLogger::InhibitOutput();
	dataloader->GetDefaultDataSetInfo().GetDataSet();
	TMVA::MsgLogger::EnableOutput();

	std::cout << "prepared DATA LOADER " << std::endl;

	Info("TMVA_CNN_Classification", "Building convolutional keras model");

	TMacro m;
	m.AddLine("import tensorflow");
	m.AddLine("from tensorflow.keras.models import Sequential");
	m.AddLine("from tensorflow.keras.optimizers import Adam");
	m.AddLine("from tensorflow.keras.layers import Input, Dense, Dropout, Flatten, Conv2D, MaxPooling2D, Reshape");
	m.AddLine("");
	m.AddLine("model = Sequential() ");
	m.AddLine("model.add(Reshape((32, 32, 1), input_shape = (1024, )))");
	m.AddLine("model.add(Conv2D(12, kernel_size = (3, 3), kernel_initializer = 'glorot_normal',activation = "
		"'relu', padding = 'same'))");
	m.AddLine("model.add(Conv2D(12, kernel_size = (3, 3), kernel_initializer = 'glorot_normal',activation = "
		"'relu', padding = 'same'))");
	m.AddLine("model.add(MaxPooling2D(pool_size = (2, 2), strides = (2,2))) ");

	m.AddLine("model.add(Conv2D(12, kernel_size = (3, 3), kernel_initializer = 'glorot_normal',activation = "
		"'relu', padding = 'same'))");
	m.AddLine("model.add(Conv2D(12, kernel_size = (3, 3), kernel_initializer = 'glorot_normal',activation = "
		"'relu', padding = 'same'))");
	m.AddLine("model.add(MaxPooling2D(pool_size = (2, 2), strides = (2,2))) ");


	m.AddLine("model.add(Flatten())");
	m.AddLine("model.add(Dense(64, activation = 'relu')) ");
	m.AddLine("model.add(Dense(32, activation = 'relu')) ");
	m.AddLine("model.add(Dense(1, activation = 'linear')) ");

	m.AddLine("model.compile(loss = 'binary_crossentropy', optimizer = Adam(lr = 0.001), metrics = ['accuracy'])");
	m.AddLine("model.save('model_cnn.h5')");
	m.AddLine("model.summary()");

	m.SaveSource("make_cnn_model.py");
	gSystem->Exec("python make_cnn_model.py");

	factory->BookMethod(
	dataloader, TMVA::Types::kPyKeras, "PyKeras",
	"H:!V:VarTransform=None:FilenameModel=model_cnn.h5:tf.keras:"
	"FilenameTrainedModel=trained_model_cnn.h5:NumEpochs=10:BatchSize=32:"
	"GpuOptions=allow_growth=True"); 


	factory->TrainAllMethods();
	factory->TestAllMethods();

	outputFile->Close();
}
