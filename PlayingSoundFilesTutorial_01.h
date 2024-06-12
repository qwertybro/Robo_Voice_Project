#pragma once

//==============================================================================
class MainContentComponent : public juce::AudioAppComponent,
    public juce::ChangeListener
{
public:
    MainContentComponent()
        : state(Stopped)

    {
        addAndMakeVisible(&openButton);
        openButton.setButtonText("Open file");
        openButton.onClick = [this] { openButtonClicked(); };

        addAndMakeVisible(&playButton);
        playButton.setButtonText("Play");
        playButton.onClick = [this] { playButtonClicked(); };
        playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
        playButton.setEnabled(false);

        addAndMakeVisible(&stopButton);
        stopButton.setButtonText("Stop");
        stopButton.onClick = [this] { stopButtonClicked(); };
        stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
        stopButton.setEnabled(false);

        addAndMakeVisible(&RoboButton);
        RoboButton.setButtonText("Effect");
        RoboButton.onClick = [this] { reverseButtonClicked(); };
        RoboButton.setColour(juce::TextButton::buttonColourId, juce::Colours::purple);
        RoboButton.setEnabled(false);

        addAndMakeVisible(&volumeSlider); 
        volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal); 
        volumeSlider.setRange(0.0, 100.0, 1.0); 
        volumeSlider.setValue(100.0); 
        volumeSlider.onValueChange = [this] { updateVolume(); };

        setSize(400, 300); 

        formatManager.registerBasicFormats();
        transportSource.addChangeListener(this);

        setAudioChannels(0, 2);
    }

    ~MainContentComponent() override
    {
        shutdownAudio();
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        if (readerSource.get() == nullptr)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        transportSource.getNextAudioBlock(bufferToFill);

        if (Robo)
        {
            for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
            {
                float* channelData = bufferToFill.buffer->getWritePointer(channel);
                std::reverse(channelData, channelData + bufferToFill.numSamples - 100);
            }
        }

        // Apply volume change
        bufferToFill.buffer->applyGain(volumeSlider.getValue() / 100.0f);
    }

    void releaseResources() override
    {
        transportSource.releaseResources();
    }

    void resized() override
    {
        openButton.setBounds(10, 10, getWidth() - 20, 20);
        playButton.setBounds(10, 40, getWidth() - 20, 20);
        stopButton.setBounds(10, 70, getWidth() - 20, 20);
        RoboButton.setBounds(10, 100, getWidth() - 20, 20);
        volumeSlider.setBounds(10, 130, getWidth() - 20, 20);
    }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {

        if (source == &transportSource)
        {
            if (transportSource.isPlaying())
                changeState(Playing);
            else if ((state == Stopping) || (state == Playing))
                changeState(Stopped);
            else if (Pausing == state)
                changeState(Paused);
        }
    }

private:
    bool Robo = false; // Начальное состояние реверса

    enum TransportState
    {
        Stopped,
        Starting,
        Playing,
        Pausing,
        Paused,
        Stopping
    };

    void changeState(TransportState newState)
    {
        if (state != newState)
        {
            state = newState;

            switch (state)
            {
            case Stopped:                           
                playButton.setButtonText("Play");
                stopButton.setButtonText("Stop");
                stopButton.setEnabled(false);
                playButton.setEnabled(true);
                RoboButton.setEnabled(false); 
                transportSource.setPosition(0.0);
                break;

            case Starting:                         

                transportSource.start();
                break;

            case Playing:                           
                playButton.setButtonText("Pause");
                stopButton.setButtonText("Stop");
                stopButton.setEnabled(true);
                RoboButton.setEnabled(true); 
                break;

            case Pausing:
                transportSource.stop();
                break;
            case Paused:
                playButton.setButtonText("Resume");
                stopButton.setButtonText("Return to Zero");
            case Stopping:                         
                transportSource.stop();
                break;
            default:
                jassertfalse;
                break;
            }
        }
    }

    void transportSourceChanged()
    {
        changeState(transportSource.isPlaying() ? Playing : Stopped);
    }

    void thumbnailChanged()
    {
        repaint();
    }

    void openButtonClicked()
    {
        chooser = std::make_unique<juce::FileChooser>("Select a Wave file to play...",
            juce::File{},
            "*.wav");                     
        auto chooserFlags = juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)     
            {
                auto file = fc.getResult();

        if (file != juce::File{})                                                
        {
            auto* reader = formatManager.createReaderFor(file);                 

            if (reader != nullptr)
            {
                auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);   
                transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);       
                playButton.setEnabled(true);                                                      
                RoboButton.setEnabled(true); 
                readerSource.reset(newSource.release());                                          
            }
        }
            });
    }

    void playButtonClicked()
    {
        
        if ((state == Stopped) || (state == Paused))
            changeState(Starting);
        else if (state == Playing)
            changeState(Pausing);
    }

    void stopButtonClicked()
    {
        
        if (state == Paused)
            changeState(Stopped);
        else
            changeState(Stopping);
    }

    void reverseButtonClicked()
    {
        Robo = !Robo; 
    }

    void updateVolume()
    {
        
    }


    //==========================================================================
    juce::TextButton openButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton RoboButton;

    juce::Slider volumeSlider; 

    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    TransportState state;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
};
