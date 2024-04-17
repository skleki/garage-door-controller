#ifndef __filtered_value_h
#define __filtered_value_h 

class FilteredValue
{
  public:
    FilteredValue()
    {
        init(1.0, 1000);
    }

    // Initializes filter with a time-window in samples. This roughly corresponds to a time constant in samples
    // Exact time constant can be calculated as t = -1 / ln((window-1)/window) - see explanation linked above
    void init(double window, int secondsRetaining)
    {
        _window = window;
		_secondsRetaining = secondsRetaining;
        _value = 0.0;
        _minValue = 0.0;
        _maxValue = 0.0;
        _start = true;
    }

	void ExecuteOn1secondInterval()
	{
		if (_valueValidForInterval > 0)
		{
			_valueValidForInterval--;
		}
	}
	

    void setWindow(double window)
    {
        _window = window;
    }

    void add(double newval)
    {
        if(_start)
        {
            reset(newval);
        }
        else
        {
            _value = ((_window - 1.0) * _value + newval) / _window;
            _minValue = (std::min)(_minValue, newval);
            _maxValue = (std::max)(_maxValue, newval);
        }
        _start = false;
		_valueValidForInterval = _secondsRetaining;
    }
    void addNonZero(double newval)
    {
        if(newval > 0.0)
        {
            add(newval);
        }
    }

    void addAndResetUp(double newval)
    {
        if(_value < newval)
        {
            _value = newval;
        }
        else
        {
            add(newval);
        }
    }

    void addAndResetDown(double newval)
    {
        if(_value > newval)
        {
            _value = newval;
        }
        else
        {
            add(newval);
        }
    }
    void reset(double newval)
    {
        _value = newval;
        _minValue = newval;
        _maxValue = newval;
    }

    bool hasValue() const 
	{
		if (_valueValidForInterval > 0)
			return !_start;
		else
			return false;
	}
	
    double getValue() const { return _value; }
    double getMinValue() const { return _minValue; }
    double getMaxValue() const { return _maxValue; }

  private:
    double _window;
	int _secondsRetaining;
	int _valueValidForInterval;
    double _value;
    double _minValue;
    double _maxValue;
    bool _start;
};

#endif