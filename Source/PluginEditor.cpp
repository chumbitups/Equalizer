/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
	int x, 
	int y, 
	int width, 
	int height,
	float sliderPosProportional,
	float rotaryStartAngle,
	float rotaryEndAngle,
	juce::Slider& slider)
{
	using namespace juce;

	auto bounds = Rectangle<float>(x, y, width, height);

	auto enabled = slider.isEnabled();

	g.setColour(enabled ? Colours::cornflowerblue : Colours::darkgrey);
	g.fillEllipse(bounds);

	g.setColour(enabled ? Colours::blueviolet : Colours::grey);
	g.drawEllipse(bounds, 1.f);

	if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
	{
		auto center = bounds.getCentre();
		Path p;

		Rectangle<float> r;
		r.setLeft(center.getX() - 2);
		r.setRight(center.getX() + 2);
		r.setTop(bounds.getY());
		r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

		p.addRoundedRectangle(r, 2.f);

		jassert(rotaryStartAngle > rotaryEndAngle);

		auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

		p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

		g.fillPath(p);

		g.setFont(rswl->getTextHeight());
		auto text = rswl->getDisplayString();
		auto strWidth = g.getCurrentFont().getStringWidth(text);

		r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
		r.setCentre(bounds.getCentre());

		g.setColour(enabled ? Colours::black : Colours::darkgrey);
		g.fillRect(r);

		g.setColour(enabled ? Colours::white : Colours::lightgrey);
		g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
	}	
}

void LookAndFeel::drawToggleButton(juce::Graphics& g,
	juce::ToggleButton& toggleButton,
	bool shouldDrawButtonAsHighlighted,
	bool shouldDrawButtonAsDown)
{
	using namespace juce;
	if (auto* pb = dynamic_cast<PowerButton*>(&toggleButton))
	{
		// Drawing power buttons
		Path powerButton;

		auto bounds = toggleButton.getLocalBounds();
		auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
		auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

		float ang = 30.f;

		size -= 6;

		powerButton.addCentredArc(r.getCentreX(),
			r.getCentreY(),
			size * 0.5,
			size * 0.5,
			0.f,
			degreesToRadians(ang),
			degreesToRadians(360.f - ang),
			true);

		powerButton.startNewSubPath(r.getCentreX(), r.getY());
		powerButton.lineTo(r.getCentre());

		PathStrokeType pst(2.f, PathStrokeType::curved);

		auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colours::khaki;

		g.setColour(color);
		g.strokePath(powerButton, pst);
		g.drawEllipse(r, 2);
	}
	else if (auto* analyzerButton = dynamic_cast<AnalyzerButton*>(&toggleButton))
	{
		// Drawing analyzer power button
		auto color = ! toggleButton.getToggleState() ? Colours::dimgrey : Colours::khaki;

		g.setColour(color);

		auto bounds = toggleButton.getLocalBounds();
		g.drawRect(bounds);

		g.strokePath(analyzerButton->randomPath, PathStrokeType(1.f));
	}
}

//==============================================================================
void RotarySliderWithLabels::paint(juce::Graphics& g)
{
	using namespace juce;

	auto startAng = degreesToRadians(180.f + 45.f);
	auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

	auto range = getRange();

	auto sliderBounds = getSliderBounds();

	getLookAndFeel().drawRotarySlider(g,
		sliderBounds.getX(),
		sliderBounds.getY(),
		sliderBounds.getWidth(),
		sliderBounds.getHeight(),
		jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
		startAng,
		endAng,
		*this);

	auto center = sliderBounds.toFloat().getCentre();
	auto radius = sliderBounds.getWidth() * 0.5f;

	// Displaying min/max values of parameters
	g.setColour(Colours::khaki);
	g.setFont(getTextHeight());

	auto numChoices = labels.size();
	for (int i = 0; i < numChoices; i++)
	{
		auto pos = labels[i].pos;
		jassert(0.f <= pos);
		jassert(pos <= 1.f);

		auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

		auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);

		Rectangle<float> r;
		auto str = labels[i].label;
		r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
		r.setCentre(c);
		r.setY(r.getY() + getTextHeight());

		g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
	}
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
	auto bounds = getLocalBounds();

	auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

	size -= getTextHeight() * 2;
	juce::Rectangle<int> r;
	r.setSize(size, size);
	r.setCentre(bounds.getCentreX(), 0);
	r.setY(2);

	return r;
}
juce::String RotarySliderWithLabels::getDisplayString() const
{
	// dB / Oct

	if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
		return choiceParam->getCurrentChoiceName();

	// Hz to kHz

	juce::String str;
	bool addK = false;
	
	if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
	{
		float val = getValue();

		if (val > 999.f)
		{
			val /= 1000.f;
			addK = true;
		}
		str = juce::String(val, (addK ? 2 : 0));
	}
	else
	{
		jassertfalse;
	}

	// Quality

	if (suffix.isNotEmpty())
	{
		str << " ";
		if (addK)
			str << "k";

		str << suffix;
	}
	return str;
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(EqualizerAudioProcessor& p) : 
audioProcessor(p), 
//leftChannelFifo(&audioProcessor.leftChannelFifo)
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo)
{
	const auto& params = audioProcessor.getParameters();
	for (auto param : params)
	{
		param->addListener(this);
	}

	updateChain();

	startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
	const auto& params = audioProcessor.getParameters();
	for (auto param : params)
	{
		param->removeListener(this);
	}
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
	parametersChanged.set(true);
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
	// Sending samples to FFT data generator
	juce::AudioBuffer<float> tempIncomingBuffer;

	while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
	{
		if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
		{
			auto size = tempIncomingBuffer.getNumSamples();

			juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
				monoBuffer.getReadPointer(0, size),
				monoBuffer.getNumSamples() - size);

			juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
				tempIncomingBuffer.getReadPointer(0, 0),
				size);

			leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
		}
	}

	// Producing Paths from FFT dataGen
	// if there are FFT data buffer to pull
	//   if we can pull a buffer
	//     generate a path
	const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
	const auto binWidth = sampleRate / (double)fftSize;  // Sample Rate / FFT size <- bin width

	while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
	{
		std::vector<float> fftData;
		if (leftChannelFFTDataGenerator.getFFTData(fftData))
		{
			pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
		}
	}

	//Pulling paths
	while (pathProducer.getNumPathsAvailable())
	{
		pathProducer.getPath(leftChannelFFTPath);
	}
}

void ResponseCurveComponent::timerCallback()
{
	if (shouldShowFFTAnalysis)
	{
		auto fftBounds = getAnalysisArea().toFloat();
		auto sampleRate = audioProcessor.getSampleRate();

		leftPathProducer.process(fftBounds, sampleRate);
		rightPathProducer.process(fftBounds, sampleRate);
	}

	//Updating the curve
	if (parametersChanged.compareAndSetBool(false, true))
	{
		updateChain();

		/*repaint();*/
	}
	repaint();
}

void ResponseCurveComponent::updateChain()
{
	auto chainSettings = getChainSettings(audioProcessor.apvts);

	monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
	monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
	monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

	auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
	updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

	auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
	auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

	updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
	updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
	// Creating and painting a response curve

	using namespace juce;
	g.fillAll(Colours::black);

	g.drawImage(background, getLocalBounds().toFloat());

	auto responseArea = getAnalysisArea();
	
	auto width = responseArea.getWidth();

	auto& lowcut = monoChain.get<ChainPositions::LowCut>();
	auto& peak = monoChain.get<ChainPositions::Peak>();
	auto& highcut = monoChain.get<ChainPositions::HighCut>();

	auto sampleRate = audioProcessor.getSampleRate();

	std::vector<double> mags;

	mags.resize(width);

	for (int i = 0; i < width; i++)
	{
		double mag = 1.f;
		auto freq = mapToLog10(double(i) / double(width), 20.0, 20000.0);

		if (!monoChain.isBypassed<ChainPositions::Peak>())
			mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

		if (!monoChain.isBypassed<ChainPositions::LowCut>())
		{
			if (!lowcut.isBypassed<0>())
				mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
			if (!lowcut.isBypassed<1>())
				mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
			if (!lowcut.isBypassed<2>())
				mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
			if (!lowcut.isBypassed<3>())
				mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		}

		if (!monoChain.isBypassed<ChainPositions::HighCut>())
		{
			if (!highcut.isBypassed<0>())
				mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
			if (!highcut.isBypassed<1>())
				mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
			if (!highcut.isBypassed<2>())
				mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
			if (!highcut.isBypassed<3>())
				mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		}
		mags[i] = Decibels::gainToDecibels(mag);
	}

	Path responseCurve;

	const double outputMin = responseArea.getBottom();
	const double outputMax = responseArea.getY();
	auto map = [outputMin, outputMax](double input)
		{
			return jmap(input, -24.0, 24.0, outputMin, outputMax);
		};
	responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

	for (size_t i = 1; i < mags.size(); i++)
	{
		responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
	}

	// Drawing Spectrum Analyzer if button is enabled
	if (shouldShowFFTAnalysis)
	{
		// Skyblue left channel
		auto leftChannelFFTPath = leftPathProducer.getPath();
		leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
		g.setColour(Colours::skyblue);
		g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));

		// Yellow right channel 
		auto rightChannelFFTPath = rightPathProducer.getPath();
		rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
		g.setColour(Colours::lightyellow);
		g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
	}

	// White rectangle with the response curve in it
	g.setColour(Colours::white);
	g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);

	// Blueviolet response curve
	g.setColour(Colours::blueviolet);
	g.strokePath(responseCurve, PathStrokeType(2.f));
}

void ResponseCurveComponent::resized()
{
	//Drawing a grid with params

	using namespace juce;
	background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);

	Graphics g(background);
	
	// Vertical lines are frequencies

	Array<float> freqs
	{
		20, /*30, 40,*/ 50, 100,
		200, /*300, 400,*/ 500, 1000,
		2000, /*3000, 4000,*/ 5000, 10000,
		20000
	};

	auto renderArea = getAnalysisArea();
	auto left = renderArea.getX();
	auto right = renderArea.getRight();
	auto top = renderArea.getY();
	auto bottom = renderArea.getBottom();
	auto width = renderArea.getWidth();

	Array <float> xs;
	for (auto f : freqs)
	{
		auto normX = mapFromLog10(f, 20.f, 20000.f);
		xs.add(left + width * normX);
	}

	g.setColour(Colours::dimgrey);
	for (auto x : xs)
	{
		g.drawVerticalLine(x, top, bottom);
	}

	// Horizontal lines are gains

	Array<float> gain
	{
		-24, -12, 0, 12, 24
	};

	for (auto gDb : gain)
	{
		auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
	
		g.setColour(gDb == 0.f ? Colours::khaki : Colours::darkgrey);
		g.drawHorizontalLine(y, left, right);
	}

	// Drawing frequency labels

	g.setColour(Colours::lightgrey);
	const int fontHeight = 10;
	g.setFont(fontHeight);

	for (int i = 0; i < freqs.size(); i++)
	{
		auto f = freqs[i];
		auto x = xs[i];

		bool addK = false;
		String str;
		if (f > 999.f) {
			f /= 1000.f;
			addK = true;
		}

		str << f;
		if (addK)
			str << "k";
		str << "Hz";

		auto textWidth = g.getCurrentFont().getStringWidth(str);

		Rectangle<int> r;
		r.setSize(textWidth, fontHeight);
		r.setCentre(x, 0);
		r.setY(1);

		g.drawFittedText(str, r, juce::Justification::centred, 1);
	}

	//Drawing gain labels

	for (auto gDb : gain)
	{
		auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
		String str;
		if (gDb > 0)
			str << "+";
		str << gDb;

		auto textWidth = g.getCurrentFont().getStringWidth(str);

		Rectangle<int> r;
		r.setSize(textWidth, fontHeight);
		r.setX(getWidth() - textWidth);
		r.setCentre(r.getCentreX(), y);

		g.setColour(gDb == 0.f ? Colours::khaki : Colours::lightgrey);

		g.drawFittedText(str, r, juce::Justification::centred, 1);

		str.clear();
		str << (gDb - 24.f);

		r.setX(1);
		textWidth = g.getCurrentFont().getStringWidth(str);
		r.setSize(textWidth, fontHeight);
		g.setColour(Colours::lightgrey);
		g.drawFittedText(str, r, juce::Justification::centred, 1);
	}
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
	auto bounds = getLocalBounds();

	bounds.removeFromTop(12);
	bounds.removeFromBottom(2);
	bounds.removeFromLeft(20);
	bounds.removeFromRight(20);

	return bounds;
}

// Render area of the grid
juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
	auto bounds = getRenderArea();
	bounds.removeFromTop(4);
	bounds.removeFromBottom(4);
	return bounds;
}

//==============================================================================
EqualizerAudioProcessorEditor::EqualizerAudioProcessorEditor(EqualizerAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p),
	peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
	peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
	peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
	lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
	highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
	lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
	highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "db/Oct"),

	responseCurveComponent(audioProcessor),
	peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
	peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
	peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
	lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
	highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
	lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
	highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),

	lowCutBypassButtonAttachment(audioProcessor.apvts, "LowCut Bypassed", lowCutBypassButton),
	peakBypassButtonAttachment(audioProcessor.apvts, "Peak Bypassed", peakBypassButton),
	highCutBypassButtonAttachment(audioProcessor.apvts, "HighCut Bypassed", highCutBypassButton),
	analyzerEnabledButtonAttachment(audioProcessor.apvts, "Analyzer Enabled", analyzerEnabledButton)
{
	peakFreqSlider.labels.add({ 0.f, "20Hz" });
	peakFreqSlider.labels.add({ 1.f, "20kHz" });

	peakGainSlider.labels.add({ 0.f, "-24dB" });
	peakGainSlider.labels.add({ 1.f, "+24dB" });

	peakQualitySlider.labels.add({ 0.f,"0.1" });
	peakQualitySlider.labels.add({ 1.f,"10.0" });

	lowCutFreqSlider.labels.add({ 0.f,"20Hz" });
	lowCutFreqSlider.labels.add({ 1.f,"20kHz" });

	highCutFreqSlider.labels.add({ 0.f,"20Hz" });
	highCutFreqSlider.labels.add({ 1.f,"20kHz" });

	lowCutSlopeSlider.labels.add({ 0.f, "12" });
	lowCutSlopeSlider.labels.add({ 1.f, "48" });

	highCutSlopeSlider.labels.add({ 0.f, "12" });
	highCutSlopeSlider.labels.add({ 1.f, "48" });


    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

	peakBypassButton.setLookAndFeel(&lnf);
	lowCutBypassButton.setLookAndFeel(&lnf);
	highCutBypassButton.setLookAndFeel(&lnf);
	analyzerEnabledButton.setLookAndFeel(&lnf);

	auto safePtr = juce::Component::SafePointer<EqualizerAudioProcessorEditor>(this);
	peakBypassButton.onClick = [safePtr]()
		{
			if (auto* comp = safePtr.getComponent())
			{
				auto bypassed = comp->peakBypassButton.getToggleState();

				comp->peakFreqSlider.setEnabled(!bypassed);
				comp->peakGainSlider.setEnabled(!bypassed);
				comp->peakQualitySlider.setEnabled(!bypassed);
			}
		};

	lowCutBypassButton.onClick = [safePtr]()
		{
			if (auto* comp = safePtr.getComponent())
			{
				auto bypassed = comp->lowCutBypassButton.getToggleState();

				comp->lowCutFreqSlider.setEnabled(!bypassed);
				comp->lowCutSlopeSlider.setEnabled(!bypassed);
			}
		};

	highCutBypassButton.onClick = [safePtr]()
		{
			if (auto* comp = safePtr.getComponent())
			{
				auto bypassed = comp->lowCutBypassButton.getToggleState();

				comp->highCutFreqSlider.setEnabled(!bypassed);
				comp->highCutSlopeSlider.setEnabled(!bypassed);
			}
		};

	analyzerEnabledButton.onClick = [safePtr]()
		{
			if (auto* comp = safePtr.getComponent())
			{
				auto enabled = comp->analyzerEnabledButton.getToggleState();
				comp->responseCurveComponent.toggleAnalysisEnablement(enabled);
			}
		};

    setSize (600, 480);
}

EqualizerAudioProcessorEditor::~EqualizerAudioProcessorEditor()
{
	peakBypassButton.setLookAndFeel(nullptr);
	lowCutBypassButton.setLookAndFeel(nullptr);
	highCutBypassButton.setLookAndFeel(nullptr);
}

//==============================================================================
void EqualizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Drawing sliders' box

    using namespace juce;
    g.fillAll(Colours::black);
}

void EqualizerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

	// Analyzer Enabled button area 
    auto bounds = getLocalBounds();

	auto analyzerEnabledArea = bounds.removeFromTop(25);
	analyzerEnabledArea.setWidth(100);
	analyzerEnabledArea.setX(5);
	analyzerEnabledArea.removeFromTop(2);

	analyzerEnabledButton.setBounds(analyzerEnabledArea);

	bounds.removeFromTop(5);

	// Response curve area
	float hRatio = 26.f / 100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);

	responseCurveComponent.setBounds(responseArea);

	bounds.removeFromTop(5);

	// Cuts area
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

	lowCutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

	highCutBypassButton.setBounds(highCutArea.removeFromTop(25));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);

	//Peaks area
	peakBypassButton.setBounds(bounds.removeFromTop(25));
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
}

//Returning components of GUI
std::vector<juce::Component*> EqualizerAudioProcessorEditor::getComps()
{
	return
	{
		&peakFreqSlider,
		&peakGainSlider,
		&peakQualitySlider,
		&lowCutFreqSlider,
		&highCutFreqSlider,
		&lowCutSlopeSlider,
		&highCutSlopeSlider,
		&responseCurveComponent,

		&lowCutBypassButton, 
		&peakBypassButton, 
		&highCutBypassButton, 
		&analyzerEnabledButton
    };
}
