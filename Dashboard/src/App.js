// src/App.js - File App.js với default export đúng cách
import React from 'react';
import MultiChannelDashboard from './components/MultiChannelDashboard';
import './App.css';

function App() {
  return (
    <div className="App">
      <MultiChannelDashboard />
    </div>
  );
}

export default App;